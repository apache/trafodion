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
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"


#include "rtdu2.h"
#include "ExpError.h"
//ss_cc_change : This file is obsolete and is not used
//LCOV_EXCL_START
// read and return module header structure
module_header_struct*
readModuleHeader
(fstream              &mf,     // (IN): binary module file
 module_header_struct &hdr,    // (IN): target of read
 const char *         name,    // (IN): name of module (for error msg)
 ComDiagsArea         &diags)  // (IN): deposit any error msg here
{
  // read the module header
  mf.seekg(0, ios::beg);
  mf.read((char *)&hdr, sizeof(module_header_struct));
  if (mf.fail()) {
    diags << DgSqlCode(-CLI_READ_ERROR) << DgString0(name);
    return NULL;
  }

  // give versioning a chance to massage/migrate it to this version
  module_header_struct modHdrCls, *latestModHdr = (module_header_struct*)
    hdr.driveUnpack(&hdr, &modHdrCls, NULL);
  if (!latestModHdr) {
    // error: version is no longer supported
    diags << DgSqlCode(-CLI_MODULE_HDR_VERSION_ERROR) 
          << DgString0(name);
    return NULL;
  }

  // verify its validity
  Lng32 errCode = latestModHdr->RtduStructIsCorrupt();
  if (errCode) {
    // the module file is corrupted or has invalid data
    diags << DgSqlCode(errCode) << DgString0(name);
    return NULL;
  }
  return latestModHdr;
}

// read and return procedure location table area (header + entries)
Int32
readPLTArea
(fstream              &mf,          // (IN) : binary module file
 module_header_struct &latestModHdr,// (IN) : its module header
 NAHeap               &heap,        // (IN) : allocate PLT area from here
 const char *     name,       // (IN) : module name (for error msg)
 ComDiagsArea         &diags,       // (IN) : deposit any error msg here
 plt_header_struct   *&pltArea)     // (OUT): plt header + entries
{
  // make sure we have reasonable arguments
  if (latestModHdr.plt_area_offset  <= 0 ||
      latestModHdr.plt_area_length  <= 0 ||
      latestModHdr.plt_hdr_length   <= 0 ||
      latestModHdr.plt_entry_length <= 0)
    return -1;

  // allocate space for PLT header
  plt_header_struct pltHdrCls, *latestPLTHdr, *plt;
  plt = (plt_header_struct *)
    heap.allocateMemory(latestModHdr.plt_hdr_length);

  // read procedure location table header
  mf.seekg(latestModHdr.plt_area_offset, ios::beg);
  mf.read((char *)plt, latestModHdr.plt_hdr_length);
  if (mf.fail()) {
    diags << DgSqlCode(-CLI_READ_ERROR) << DgString0(name);
    return -1;
  }
      
  // give versioning a chance to massage/migrate it to this version
  latestPLTHdr = (plt_header_struct*)plt->driveUnpack(plt, &pltHdrCls,NULL);
  if (!latestPLTHdr) {
    // error: version is no longer supported
    diags << DgSqlCode(-CLI_MOD_PLT_HDR_VERSION_ERROR) 
          << DgString0(name);
    return -1;
  }

  pltArea = latestPLTHdr;
  Int32 num_procs = latestPLTHdr->num_procedures;
  
  if (num_procs >= 1) {
	// allocate space for PLT header + entries
    heap.deallocateMemory(plt);
    plt = (plt_header_struct *)
      heap.allocateMemory((size_t)latestModHdr.plt_area_length);
	  
    // read procedure location table header + entries
    mf.seekg(latestModHdr.plt_area_offset, ios::beg);
    mf.read((char *)plt, (Int32)latestModHdr.plt_area_length);
    if (mf.fail()) {
      diags << DgSqlCode(-CLI_READ_ERROR) << DgString0(name);
      return -1;
    }
	  
    // give versioning a chance to massage/migrate it to this version
    latestPLTHdr = (plt_header_struct*)plt->driveUnpack(plt, &pltHdrCls, NULL);
    if (!latestPLTHdr) {
      // error: version is no longer supported
      diags << DgSqlCode(-CLI_MOD_PLT_HDR_VERSION_ERROR) 
            << DgString0(name);
      return -1;
    }
    pltArea = latestPLTHdr;
  }

  // verify its validity
  Lng32 errCode = pltArea->RtduStructIsCorrupt();
  if (errCode) {
    // the module file is corrupted or has invalid data
    diags << DgSqlCode(errCode) << DgString0(name);
    heap.deallocateMemory(plt);
    return -1;
  }
  return num_procs;
}

// read and return descriptor location table area (header + entries)
Int32
readDLTArea
(fstream              &mf,          // (IN) : binary module file
 module_header_struct &latestModHdr,// (IN) : its module header
 NAHeap               &heap,        // (IN) : allocate DLT area from here
 const char *     name,       // (IN) : module name (for error msg)
 ComDiagsArea         &diags,       // (IN) : deposit any error msg here
 dlt_header_struct   *&dltArea)     // (OUT): dlt header + entries
{
  // make sure we have reasonable arguments
  if (latestModHdr.dlt_area_offset  <= 0 ||
      latestModHdr.dlt_area_length  <= 0 ||
      latestModHdr.dlt_hdr_length   <= 0 ||
      latestModHdr.dlt_entry_length <= 0)
    return -1;

  // allocate space for DLT header
  dlt_header_struct dltHdrCls, *latestDLTHdr, *dlt;
  dlt = (dlt_header_struct *)
    heap.allocateMemory(latestModHdr.dlt_hdr_length);

  // read descriptor location table header
  mf.seekg(latestModHdr.dlt_area_offset, ios::beg);
  mf.read((char *)dlt, latestModHdr.dlt_hdr_length);
  if (mf.fail()) {
    diags << DgSqlCode(-CLI_READ_ERROR) << DgString0(name);
    return -1;
  }
      
  // give versioning a chance to massage/migrate it to this version
  latestDLTHdr = (dlt_header_struct*)dlt->driveUnpack(dlt, &dltHdrCls, NULL);
  if (!latestDLTHdr) {
    // error: version is no longer supported
    diags << DgSqlCode(-CLI_MOD_DLT_HDR_VERSION_ERROR) 
          << DgString0(name);
    return -1;
  }

  dltArea = latestDLTHdr;
  Int32 num_descs = latestDLTHdr->num_descriptors;
  
  if (num_descs >= 1) {
	// allocate space for DLT header + entries
    heap.deallocateMemory(dlt);
    dlt = (dlt_header_struct *)
      heap.allocateMemory((size_t)latestModHdr.dlt_area_length);
	  
    // read descriptor location table header + entries
    mf.seekg(latestModHdr.dlt_area_offset, ios::beg);
    mf.read((char *)dlt, (Int32)latestModHdr.dlt_area_length);
    if (mf.fail()) {
      diags << DgSqlCode(-CLI_READ_ERROR) << DgString0(name);
      return -1;
    }
	  
    // give versioning a chance to massage/migrate it to this version
    latestDLTHdr = (dlt_header_struct*)dlt->driveUnpack(dlt, &dltHdrCls, NULL);
    if (!latestDLTHdr) {
      // error: version is no longer supported
      diags << DgSqlCode(-CLI_MOD_DLT_HDR_VERSION_ERROR) 
            << DgString0(name);
      return -1;
    }
    dltArea = latestDLTHdr;
  }

  // verify its validity
  Lng32 errCode = dltArea->RtduStructIsCorrupt();
  if (errCode) {
    // the module file is corrupted or has invalid data
    diags << DgSqlCode(errCode) << DgString0(name);
    heap.deallocateMemory(dlt);
    return -1;
  }
  return num_descs;
}

// read and return descriptor area (headers + entries)
Int32
readDescArea
(fstream              &mf,          // (IN) : binary module file
 module_header_struct &latestModHdr,// (IN) : its module header
 NAHeap               &heap,        // (IN) : allocate DESC area from here
 const char *         name,       // (IN) : module name (for error msg)
 ComDiagsArea         &diags,       // (IN) : deposit any error msg here
 char                *&descArea)    // (OUT): desc headers + entries
{
  // make sure we have reasonable arguments
  if (latestModHdr.descriptor_area_offset <= 0 ||
      latestModHdr.descriptor_area_length <= 0)
    return -1;

  // allocate space for DESC headers + entries
  descArea = (char*)
    heap.allocateMemory((size_t)latestModHdr.descriptor_area_length);
	  
  // read DESC headers + entries
  mf.seekg(latestModHdr.descriptor_area_offset, ios::beg);
  mf.read(descArea, (Int32)latestModHdr.descriptor_area_length);
  if (mf.fail()) {
    diags << DgSqlCode(-CLI_READ_ERROR) << DgString0(name);
    return -1;
  }
  return 1; // all OK
}

// read and return a binary module file's source area
SourceBuf readSourceArea
(fstream               &mf,          // (IN) : binary module file
 module_header_struct  &latestModHdr,// (IN) : its module header
 NAHeap                &heap,        // (IN) : allocate PLT area from here
 const char *          name,         // (IN) : module name (for error msg)
 ComDiagsArea          &diags)       // (IN) : deposit any error msg here
{
  SourceBuf srcArea = NULL;
  if (latestModHdr.source_area_length > 0) {
    // allocate space for source area
    srcArea = (SourceBuf)
      heap.allocateMemory((size_t)latestModHdr.source_area_length);

    // read source area
    mf.seekg(latestModHdr.source_area_offset, ios::beg);
    mf.read(srcArea, (Int32)latestModHdr.source_area_length);
    if (mf.fail()) {
      diags << DgSqlCode(-CLI_READ_ERROR) << DgString0(name);
      return NULL;
    }
  }
  return srcArea;
}

//LCOV_EXCL_STOP

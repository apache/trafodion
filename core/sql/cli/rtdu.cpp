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
#include "rtdu.h"
#include "ComVersionDefs.h"

// StaticCompiler.cpp's code to create and initialize a sql module's
//   plt_header_struct, plt_entry_struct,
//   dlt_header_struct, dlt_entry_struct,
//   desc_header_struct, desc_entry_struct
// is constructor-less: it simply allocates space for each struct
// and later computes that struct's contents, field by field. Their
// private members such as flags_, classID_, versionID_, and so on
// remain uninitialized because these struct's constructors are 
// never called. So, we use these functions to initialize the
// hidden versioning fields.


void module_header_struct::initialize()
{
  prep_timestamp = 0;
  compile_timestamp = 0;

  strncpy(eye_catcher, MODULE_EYE_CATCHER, EYE_CATCHER_LEN);
  module_length = 0;
  flags = 0;
  
  num_areas = 7;                  // this is quite true -- see rtdu.h

  plt_area_length = 0;
  plt_area_offset = NULL_RTDU_OFFSET;
  plt_hdr_length   = sizeof(plt_header_struct);
  plt_entry_length = sizeof(plt_entry_struct);
  
  source_area_length = 0;
  source_area_offset = NULL_RTDU_OFFSET;
  
  recomp_control_area_length = 0;
  recomp_control_area_offset = NULL_RTDU_OFFSET;
  
  schema_names_area_length = 0;
  schema_names_area_offset = NULL_RTDU_OFFSET;
  
  dlt_area_length = 0;
  dlt_area_offset = NULL_RTDU_OFFSET;
  dlt_hdr_length   = sizeof(dlt_header_struct);
  dlt_entry_length = sizeof(dlt_entry_struct);
  
  descriptor_area_length = 0;
  descriptor_area_offset = NULL_RTDU_OFFSET;
  desc_hdr_length   = sizeof(desc_header_struct);
  desc_entry_length = sizeof(desc_entry_struct);
  
  object_area_length = 0;
  object_area_offset = NULL_RTDU_OFFSET;
  
  vproc_area_length = 0;
  vproc_area_offset = NULL_RTDU_OFFSET;
  
  memset(module_name, 0, sizeof(module_name));
  version_ = ComVersion_GetCurrentPlanVersion();

  memset(filler_, 0, sizeof(filler_));
}

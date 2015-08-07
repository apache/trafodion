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

// SQL/MP SQLCA

#ifndef __SQLCA_H
#define __SQLCA_H

enum 
{
  SQL_NUM_ERROR_ENTRIES = 8,
  SQL_PARAMS_BUF_SIZE   = 180
};

struct sql_error_struct
{
  Lng32 errcode;
  
  Lng32 subsystem_id;
  
  Lng32 param_offset;
  
  Lng32 params_count;
};

struct sqlca_struct
{
  Lng32 num_errors;
  
  Lng32 params_buffer_len;
  
  sql_error_struct errors[SQL_NUM_ERROR_ENTRIES];

  char params_buffer[SQL_PARAMS_BUF_SIZE];
};

#endif // __SQLCA_H

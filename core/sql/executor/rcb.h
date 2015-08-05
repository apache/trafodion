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
struct field_struct
{
  char flags;
  char type;
  
  short char_len;
  
  short exploded_offset;
  short offset_ix;
  
  short default_offset;
  
};



struct rcb_struct
{
  short eye_catcher;
  short rcblen;
  short fcbnum;
  short kcbnum;
  
  short vsn[3];
  
  short kfield_off;
  
  short last_index_col_ix;
  
  short signed_kfield_off;
  
  char signed_kfield_max;
  char unsigned_dec_kfield_flg;
  
  short flags2;
  
  short sect_len;
  
  short ttag;
  short tflags;
  
  short max_packed_reclen;
  
  short max_exp_reclen;
  short flags;
  short default_area_len;
  short num_entries;
  
  field_struct field[1];
}; // rcb_struct

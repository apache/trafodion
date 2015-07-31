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

#ifndef MARSHALING_H
#define MARSHALING_H

#include <odbccommon.h>
#include <odbcsrvrcommon.h>
#include <odbcas_sv.h>

typedef struct LIST_def_seq_ {
  IDL_unsigned_long _length;
  char pad_to_offset_8_[4];
  void *_buffer;
  IDL_PTR_PAD(_buffer, 1)
} LIST_def;

typedef struct OCTET_def_seq_ {
  IDL_unsigned_long _length;
  char pad_to_offset_8_[4];
  IDL_octet *_buffer;
  IDL_PTR_PAD(_buffer, 1)
} OCTET_def;



CEE_status 
decodeParameters(short count, long* param[], char* buffer, long length);

void 
LIST_length( LIST_def* pname, long length, long& wlength, long& maplength);
void 
OCTET_length( OCTET_def* pname, long& wlength, long& maplength);
void 
STRING_length( IDL_string pname,  long& wlength, long& maplength);
void 
ERROR_DESC_LIST_length( ERROR_DESC_LIST_def* pname,  long& wlength, long& maplength );
void 
SRVR_CONTEXT_length( const SRVR_CONTEXT_def* pname,  long& wlength, long& maplength );
void 
DATASOURCE_CFG_LIST_length( const DATASOURCE_CFG_LIST_def* pname,  long& wlength, long& maplength );
void 
SQLVALUE_LIST_length( const SQLValueList_def* pname,  long& wlength, long& maplength );
void
SRVR_STATUS_LIST_length( const SRVR_STATUS_LIST_def* pname, long& wlength, long& maplength );
void 
ENV_DESC_LIST_length( const ENV_DESC_LIST_def* pname, long& wlength, long& maplength );
void 
RES_DESC_LIST_length( const RES_DESC_LIST_def* pname, long& wlength, long& maplength );
void 
DATASOURCE_CFG_length( const DATASOURCE_CFG_def* pname, long& wlength, long& maplength );


void 
IDL_charArray_copy(const IDL_char* pname, char*& curptr);
void 
IDL_long_copy(IDL_long* pname, char*& curptr);
void 
IDL_unsigned_long_copy(IDL_unsigned_long* pname, char*& curptr);
void 
IDL_short_copy(IDL_short* pname, char*& curptr);
void 
IDL_unsigned_short_copy(IDL_unsigned_short* pname, char*& curptr);
void 
STRING_copy( char* buffer, IDL_string pname, IDL_string* parptr, char*& curptr, long*& mapptr);
void 
OCTET_copy( char* buffer, OCTET_def* pname, OCTET_def* parptr, char*& curptr, long*& mapptr);
void 
LIST_copy( char* buffer, LIST_def* pname, LIST_def* parptr, long length, char*& curptr, long*& mapptr);
void 
ERROR_DESC_LIST_copy( char* buffer, ERROR_DESC_LIST_def* pname, ERROR_DESC_LIST_def* parptr, char*& curptr, long*& mapptr);
void 
SRVR_CONTEXT_copy( char* buffer, const SRVR_CONTEXT_def* pname, SRVR_CONTEXT_def* parptr, char*& curptr, long*& mapptr);
void 
DATASOURCE_CFG_LIST_copy( char* buffer, const DATASOURCE_CFG_LIST_def* pname, DATASOURCE_CFG_LIST_def* parptr, char*& curptr, long*& mapptr);
void 
SQLVALUE_LIST_copy( char* buffer, const SQLValueList_def* pname, SQLValueList_def* parptr, char*& curptr, long*& mapptr);
void
SRVR_STATUS_LIST_copy(char* buffer, const SRVR_STATUS_LIST_def* pname, SRVR_STATUS_LIST_def* parptr, char*& curptr, long*& mapptr);
void 
ENV_DESC_LIST_copy(char* buffer,  const ENV_DESC_LIST_def* pname, ENV_DESC_LIST_def* parptr, char*& curptr, long*& mapptr);
void 
RES_DESC_LIST_copy(char* buffer,  const RES_DESC_LIST_def* pname, RES_DESC_LIST_def* parptr, char*& curptr, long*& mapptr);
void 
DATASOURCE_CFG_copy( char* buffer, const DATASOURCE_CFG_def* pname, DATASOURCE_CFG_def* parptr, char*& curptr, long*& mapptr);

#endif

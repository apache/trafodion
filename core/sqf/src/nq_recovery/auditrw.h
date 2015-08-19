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

#ifndef _AUDITRW_H_
#define _AUDITRW_H_

#pragma pack(push, tmf, 1) /*TMF uses one byte alignment */
#pragma pack(push, one, 4)
#include "../../inc/rosetta/rosgen.h" /* rosetta utilities */
#pragma pack(pop, one)

#ifndef int_32
#define int_32 int
#endif
#define Addr void*

struct AuditTrailPosition_Struct {
  int_32  Sequence;
  // The file sequence number.

  fixed_0 rba;
  // The Relative Byte Address within the file.
};

enum {AuditFile_MinLegalSequence = 1};
enum {AuditFile_MaxLegalSequence = 999999}; // inside the AT file name "XXnnnnnn"

enum {AuditFile_MinLegalRba = 0};
#define AuditFile_MaxLegalRba 0x10000000000ll
//#define AuditFile_MaxLegalRba (1024 * 1024 * 1024 * 1024)    // 1TB limitation

enum {AudDeblock_ReadForward = 1};
enum {AudDeblock_ReadBackward = 2};
enum {TLOG_AuditTrailIndex = 0};


extern short AuditRW_module_init (char* adpname, SB_Phandle_Type *adpphandle,
                           char *volname, short volid, short ATIndex);
// adpname, IN, the adp name like "$TLOG", '\0' expected, nor more than 8 characters
// adpphanlde, IN, will copy 20 bytes from adpphandle to store the phandle for the ADP
// and store it in global location
// vol-name, vol-id, IN, audit generator's name and id, DTM will use "$DTMnn   " as
// volume name and nn as vol-id

short AuditRW_UpdatePinningPosn(int Atindex, char *AseName, char * DtmName, int_32 SeqNo);

void AuditRW_module_terminate();

void AuditRW_activate_cursor (Addr *CursorAddr, short AuditTrailIndex,
      AuditTrailPosition_Struct *LowPos, AuditTrailPosition_Struct *HighPos,
      short Direction, short RestoresAllowed, short RestoreAheadAllowed);
// CursorAddr, OUT, address of cursor to retrieve audit records
// AuditTrailIndex, IN, which audit trail, in DTM case, it will be a dedicated num like 1
// LowPos, HighPos, IN, if the sequence = 0, indicating use wants to read from current EOT
// or the possible BOT
// Direction, IN, read forward or backward
// RestoresAllowed, RestoreAheadAllowed, IN, reserved for future AT restore, now no use
// and pass 0 instead


void AuditRW_deactivate_cursor (Addr *CursorAddr);
// CursorAddr, IN, cursor to be used to retrieve audit record


void AuditRW_read_auditrecord(Addr CursorAddr, Addr *AuditRecordAddr, short *HitEOF,
      AuditTrailPosition_Struct  *auditrecordposition);
// CursorAddr, IN, cursor to be used to retrieve audit record
// AuditRecordAddr, OUT, Address of the retrieved audit record
// HitEOF, OUT, 1 means hit the endpoint of given cursor otherwise 0
//auditrecordposition, OUT, audit trail position of the returned audit record


void AuditRW_ReleaseRecord(Addr *AuditRecordAddr);
// AuditRecordAddr, I/O, the address of record to be released


short AuditRW_send_audit(int_64 vsn, char *buffer, unsigned short bufferlen,
      AuditTrailPosition_Struct *bufferpos, short ForceFlag);
// vsn, IN, the VSN of last record in the buffer --> note. Each DTM has itw own vol-id,
// vol-name, and vsn
// buffer, IN, the buffer to be written
// bufferlen, IN, the number of bytes to write from buffer
// bufferpos, OUT, location in TLOG where the beginning of the buffer landed
// ForcedFlag, IN, 1:forced write in ADP, 0:no forced write in ADP

#pragma pack(pop, tmf) /* TMF used one byte alignment */
#endif // _AUDITRW_H_

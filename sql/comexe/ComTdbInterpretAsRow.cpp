/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2005-2014 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
/* -*-C++-*-
****************************************************************************
*
* File:         ComTdbInterpretAsRow.cpp
* Description:  
*
* Created:      6/1/2005
* Language:     C++
*
*
*
*
****************************************************************************
*/

// LCOV_EXCL_START
// Not used currently -- keep in case we ever support online populate index

#include "ComTdbInterpretAsRow.h"
#include "ComTdbCommon.h"
#include "ComQueue.h"
#include "str.h"

///////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
////////////////////////////////////////////////////////////////////////
ComTdbInterpretAsRow::ComTdbInterpretAsRow (void) :
                    ComTdb(ex_INTERPRET_AS_ROW, eye_INTERPRET_AS_ROW)
{
   extractExpr_ = 0;
   auditRowImageLen_ = 0;
   extractedRowLen_ = 0;
   outputRowLen_ = 0;
   value_ = 0;
}

ComTdbInterpretAsRow::ComTdbInterpretAsRow (UInt32 compressionFlag,
                                            UInt32 extractedRowLen,
                                            UInt32 auditImageRowLen,
                                            UInt32 outputRowLen,
                                            Cardinality estimatedRowCount,
                                            ex_cri_desc *work_cri_desc,
                                            ex_cri_desc *given_cri_desc,
                                            ex_cri_desc *returned_cri_desc,
                                            ex_expr *extractExpr,
                                            ex_expr *scanExpr,
                                            ex_expr *projExpr,
                                            queue_index upQueueSize,
                                            queue_index downQueueSize,
                                            Lng32 numBuffers,
                                            ULng32 bufferSize) :
                    ComTdb(ex_INTERPRET_AS_ROW, eye_INTERPRET_AS_ROW,
                           estimatedRowCount, given_cri_desc,
                           returned_cri_desc,
                           downQueueSize,
                           upQueueSize,
                           numBuffers,
                           bufferSize)
{
   workCriDesc_ = work_cri_desc;
   scanExpr_ = scanExpr;
   projExpr_ = projExpr;
   extractExpr_ = extractExpr;
   auditRowImageLen_ =  auditImageRowLen;
   extractedRowLen_ = extractedRowLen;
   outputRowLen_ = outputRowLen;
   value_ = 0;
   flags_.auditCompressionFlag_ = compressionFlag;
}

ComTdbInterpretAsRow::~ComTdbInterpretAsRow(void)
{
}

Long ComTdbInterpretAsRow::pack(void * space)
{
   scanExpr_.pack(space);
   projExpr_.pack(space);
   extractExpr_.pack(space);
   workCriDesc_.pack(space);

   return ComTdb::pack(space);
}

Lng32 ComTdbInterpretAsRow::unpack(void * base, void * reallocator)
{
   if (scanExpr_.unpack(base, reallocator)) return -1;
   if (projExpr_.unpack(base, reallocator)) return -1;
   if (extractExpr_.unpack(base, reallocator)) return -1;
   if (workCriDesc_.unpack(base, reallocator)) return -1;

   return ComTdb::unpack(base, reallocator);
}

void ComTdbInterpretAsRow::displayContents(Space *space, ULng32 flag)
{
   ComTdb::displayContents(space, flag & 0xFFFFFFFE);
   if (flag & 0x00000008)
   {
      char buf[100];
      str_sprintf(buf,"\nFor ComTdbInterpretAsRow: ");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf,"Audit compression flag = %d", flags_.auditCompressionFlag_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf, "AuditRowImageLen = %d, extractedRowLen = %d", auditRowImageLen_, extractedRowLen_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
   }

   if (flag & 0x00000001)
   {
      displayExpression(space, flag);
      displayChildren(space, flag);
   }
}

// LCOV_EXCL_STOP

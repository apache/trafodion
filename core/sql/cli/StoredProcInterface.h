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
****************************************************************************
*
* File:         StoredProcInterface.h (previosuly /executor/ExSPInterface.h)
* Description:  Interface file to ARKCMP for SP related stuff
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#ifndef SP_INTERFACE_H_
#define SP_INTERFACE_H_

#include "BaseTypes.h"
#include "SqlCliDllDefines.h"

class ComDiagsArea;

/////////////////////////////////////////////////////////////
// See executor/ex_stored_proc.cpp for details about these
// procs...
////////////////////////////////////////////////////////////
short ExSPPrepareInputBuffer(void * inputBuffer);

short ExSPPosition(void * inputBuffer);

short ExSPGetInputRow(void * inputBuffer,   // IN:  input sql buffer
		      void* &controlInfo,   // OUT: control info 
		      char* &rowPtr,        // OUT: pointer to the row
		      ULng32 &rowLen);// OUT: length of returned row

short ExSPInitReplyBuffer(void * replyBuffer, 
			  ULng32 replyBufLen);

short ExSPPutReplyRow(void * replyBuffer,     // IN: the reply buffer
		      void * controlInfo,     // IN: control info
		      char * replyRow,        // IN: pointer to reply row
		      ULng32 rowLen,   // IN: length of reply row
		      ComDiagsArea* diagsDesc);// IN: pointer to diags

short ExSPPrepareReplyBuffer(void * replyBuffer);

short ExSPUnpackIOExpr(void * & extractInputExpr,
		       void * & moveOutputExpr,
		       CollHeap * heap);

short ExSPExtractInputValue(void * extractInputExpr,
			    ULng32 fieldNum, char * inputRow,
			    char * data, ULng32 datalen,
                            NABoolean casting, // if TRUE,data in varchar, to be casted
			    ComDiagsArea * diagsArea);

short ExSPMoveOutputValue(void * moveOutputExpr,
			  ULng32 fieldNum, char * outputRow,
			  char * data, ULng32 datalen,
                          NABoolean casting, // if TRUE, data in varchar, to be casted
			  ComDiagsArea * diagsArea,
                          CollHeap * heap);


#endif


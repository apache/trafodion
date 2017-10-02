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
* File:         StoredProcInterface.cpp (previously part of
*                                        /executor/ex_stored_proc.cpp)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#include "Platform.h"


#include "StoredProcInterface.h"
#include "sql_buffer.h"
#include "SqlCliDllDefines.h"
#include "ComTdbStoredProc.h"
  
///////////////////////////////////////////////////////////////
// Prepares the input buffer so input rows could be retrieved
// from it. Unpacks sql buffer which would convert offsets to
// pointers. To be called after receiving the input buffer
// from Executor and before retrieving rows from it.
///////////////////////////////////////////////////////////////
short ExSPPrepareInputBuffer(void * inputBuffer)
{
  SqlBuffer * ib = (SqlBuffer *)inputBuffer;
  if (ib == NULL)
    return -1;

  ib->driveUnpack();
  
  return 0;
}

/////////////////////////////////////////////////////////////////
// Positions to the first row in input buffer. This proc MUST be
// called before retrieving input rows from it.
// RETURNS: 0, if all ok. -1, in case of an error.
/////////////////////////////////////////////////////////////////
short ExSPPosition(void * inputBuffer)
{
  SqlBuffer * ib = (SqlBuffer *)inputBuffer;
  if (ib == NULL)
    return -1;
  
  ib->position();

  return 0;
}

///////////////////////////////////////////////////////////////////////////
// This method is used to retrieve input rows from the input buffer
// that is sent from Executor to Arkcmp.
//
// Returns the 'current' row and increments the internal rownum.
//
// Also returns control information associated with this input row.
// Caller needs to pass this control info pointer to ExSPPutReplyRow 
// when the reply buffer (sent from Arkcmp to Executor) is being filled
// in. All reply(output) rows that correspond to this input row have
// the same control info.
//
// RETURNS: 0, if all is Ok. 1, if all rows have been returned.
//          -1, in case of error.
////////////////////////////////////////////////////////////////////////////
short ExSPGetInputRow(void * inputBuffer,   // IN:  input sql buffer
		      void* &controlInfo,   // OUT: control info 
		      char* &rowPtr,        // OUT: pointer to the row
		      ULng32 &rowLen)// OUT: length of returned row
{
  SqlBuffer * ib = (SqlBuffer *)inputBuffer;
  if ( !ib )
	  return -1;

  tupp tp;
  ControlInfo * ci = NULL;
  up_state us;
  ComDiagsArea* diags;
  if (ib->moveOutSendOrReplyData(TRUE, // send (input) value
				 &us,
				 tp,
				 &ci, &diags) == TRUE)
    return 1; // No more input rows.
  
  controlInfo = ci;
  
  rowPtr = tp.getDataPointer();
  rowLen = tp.getAllocatedSize();
  
  return 0;
}



/////////////////////////////////////////////////////////////////
// Initializes the reply buffer.
// Caller must allocate contiguous space of length replyBufLen
// and then call this proc. 
// This proc MUST be called BEFORE moving any reply rows to it.
// Once the reply buffer is sent from Arkcmp to Executor, it
// must be re-initialized by calling this proc again.
/////////////////////////////////////////////////////////////////
short ExSPInitReplyBuffer(void * replyBuffer, 
			  ULng32 replyBufLen)
{
  SqlBuffer * rb = (SqlBuffer *)replyBuffer;
  if ((rb == NULL) || (replyBufLen == 0))
    return -1;
  
  rb->driveInit(replyBufLen, FALSE, SqlBuffer::NORMAL_);
  
  return 0;
}

/////////////////////////////////////////////////////////////////
// Copies the row pointer by rowPtr for rowLen inside sqlBuffer.
// The controlInfo must be the same pointer that was returned by
// proc ExSPGetInputRow along with the input row that corresponds
// to the reply rows.
// If replyRow is NULL, that that indicates that all reply rows
// for this input have been returned. A NULL replyRow MUST be
// passed in to this proc to end the request.
//
// This proc returns a value of 1, if replyBuffer is full.
// The reply row that was passed in in that call is NOT moved
// in to the replyBuffer. Caller must call this proc again with
// the replyRow. This applies to both non-null and null reply row.
//
// Returns 1, if buffer is full. 0, if row moved in.
//        -1, in case of error.
//         2 in case of warning. 
/////////////////////////////////////////////////////////////////
short ExSPPutReplyRow(void * replyBuffer,     // IN: the reply buffer
		      void * controlInfo,     // IN: control info
		      char * replyRow,        // IN: pointer to reply row
		      ULng32 rowLen,   // IN: length of reply row
		      ComDiagsArea* diagsDesc)// IN: pointer to diags
{
  if ((replyBuffer == NULL) || (controlInfo == NULL))
    return -1;

  SqlBuffer  * rb = (SqlBuffer *)replyBuffer;
  ControlInfo * ci = (ControlInfo *)controlInfo;
  
  short rc = 0;
  up_state us;
  us.parentIndex = ci->getDownState().parentIndex;
  us.downIndex = 0;
  us.setMatchNo(0);
      
  if (replyRow == NULL)  // indicate end of reply rows
    {
      if (diagsDesc != NULL) 
	{
	  if (diagsDesc->getNumber(DgSqlCode::ERROR_) > 0)
	    us.status = ex_queue::Q_SQLERROR;
	  else
	    if (diagsDesc->getNumber(DgSqlCode::WARNING_) > 0)
	      {
		us.status = ex_queue::Q_NO_DATA;
		rc = 2;
	      }
	 
	}
      else
	us.status = ex_queue::Q_NO_DATA;

      tupp_descriptor* dDesc = 0;
      
      if (rb->moveInSendOrReplyData(FALSE,  // reply
				    TRUE,   // do move EOD indication 
				    FALSE,  // don't move data.
				    (void *)&us,
				    sizeof(ControlInfo),
				    0,
				    0,
				    0, 
				    diagsDesc,
				    &dDesc) == SqlBuffer::MOVE_SUCCESS)
	{
	  if (diagsDesc)
	    diagsDesc->packObjIntoMessage(dDesc->getTupleAddress());
	}
      else
	{
	  rc = 1; // buffer is full
	}
    }
  else
    {
      tupp_descriptor * tdesc = NULL;
      us.status = ex_queue::Q_OK_MMORE;
      if (rb->moveInSendOrReplyData(FALSE, // reply
				    FALSE, // don't move control,if not needed.
				    TRUE,  // do move data.
				    (void *)&us,
				    sizeof(ControlInfo),
				    0,
				    rowLen,
				    &tdesc,
				    0,
				    0) == SqlBuffer::MOVE_SUCCESS)
	{
	  str_cpy_all(tdesc->getTupleAddress(), replyRow, rowLen);
	}
      else
	{
	  rc = 1;// buffer is full.
	}
    }

  return rc;
}


///////////////////////////////////////////////////////////
// Prepares the reply buffer so it could be sent back
// from Arkcmp to Executor.
// Packs sql buffer which would convert pointers to
// offsets. To be called before sending the reply buffer to
// Executor.
///////////////////////////////////////////////////////////
short ExSPPrepareReplyBuffer(void * replyBuffer)
{
  SqlBuffer * rb = (SqlBuffer *)replyBuffer;
  if (rb == NULL)
    return -1;
  
  rb->drivePack();

  // this reply buffer needs to be sent back to executor.
  // Mark it as being in use.
  rb->bufferInUse();
  
  return 0;
}

short ExSPUnpackIOExpr(void * & extractInputExpr,
		       void * & moveOutputExpr,
		       CollHeap * heap)
{
  ExSPInputOutput *ie = (ExSPInputOutput *)extractInputExpr;

  // Enable version migration here !!!
  // pass heap to driveUnpack. TBD.

  ExSPInputOutput dummySPIO;
  if ( (ie = (ExSPInputOutput *)
        ie->driveUnpack(extractInputExpr,&dummySPIO,NULL)) == NULL )
  {
    // ERROR during unpacking. Most likely version-not-supported.
    return -1;
  }
  else
  {
    // extractInputExpr might change due to relocation during a version
    // upgrade.
    //
    extractInputExpr = ie;
  }

  ExSPInputOutput *oe = (ExSPInputOutput *)moveOutputExpr;

  if ( (oe = (ExSPInputOutput *)
        oe->driveUnpack(moveOutputExpr,&dummySPIO,NULL)) == NULL )
  {
    // ERROR during unpacking. Most likely version-not-supported.
    return -1;
  }
  else
  {
    // moveOutputExpr might change due to relocation during a version
    // upgrade.
    //
    moveOutputExpr = oe;
  }

  return 0;
}

short ExSPExtractInputValue(void * extractInputExpr,
			    ULng32 fieldNum, char * inputRow,
			    char * data, ULng32 datalen, NABoolean casting,
			    ComDiagsArea * diagsArea)
{
  return ((ExSPInputOutput *)extractInputExpr)->
    inputValue(fieldNum, inputRow,
	       data, datalen, casting, diagsArea
	       );
}

short ExSPMoveOutputValue(void * moveOutputExpr,
			  ULng32 fieldNum, char * outputRow,
			  char * data, ULng32 datalen, NABoolean casting,
			  ComDiagsArea * diagsArea,
                          CollHeap * heap)
{
  return ((ExSPInputOutput *)moveOutputExpr)->
    outputValue(fieldNum, outputRow, data, datalen, casting, heap, diagsArea);
}

 

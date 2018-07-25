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
 * File:         sql_buffer.cpp
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

//
// NOTE: Some of the code in this file is excluded from the UDR server
// build using the UDRSERV_BUILD macro. The excluded code is:
//  - All of the sql_buffer_pool, SqlBufferOlt, and SqlBufferOltSmall code
//  - Code in the SqlBuffer class that deals with ATPs, expression
//    evaluation, and statistics
//

#ifdef UDRSERV_BUILD
#include "sql_buffer.h"
#else
#include "Platform.h"
#include "ex_stdh.h"
#include "str.h"
#include "ComQueue.h"
#include "exp_expr.h"
#include "ComTdb.h"
#include "ExStats.h"
#endif // UDRSERV_BUILD
#include "sql_buffer_size.h"

// to use msg_mon_dump_process_id
#include "seabed/ms.h"

//
// Define a local assert macro. We define UdrExeAssert to something
// other than ex_assert in the UDR Server build.
//
#ifdef UDRSERV_BUILD
extern void udrAssert(const char *, Int32, const char *);
#define UdrExeAssert(p, msg) \
  if (!(p)) { udrAssert( __FILE__ , __LINE__ , msg); }
#else
#define UdrExeAssert(a,b) ex_assert(a,b)
#endif // UDRSERV_BUILD

////////////////////////////////////////////////////////////////////////////
// class SqlBufferBase
////////////////////////////////////////////////////////////////////////////
SqlBufferBase::SqlBufferBase(BufferType bufType)
     : SqlBufferHeader(bufType),
       baseFlags_(0),
       sizeInBytes_(0)
{
};

void SqlBufferBase::init(ULng32 in_size_in_bytes,
			 NABoolean clear)
{
  baseFlags_ = 0;
  sizeInBytes_      = in_size_in_bytes;

  SqlBufferHeader::init();
}

void SqlBufferBase::init()
{
  baseFlags_ = 0;

  SqlBufferHeader::init();
}

void SqlBufferBase::driveInit(ULng32 in_size_in_bytes, 
			      NABoolean clear,
			      BufferType bt)
{
  setBufType(bt);
  fixupVTblPtr();
  init(in_size_in_bytes, clear);
}

void SqlBufferBase::driveInit() 
{
  fixupVTblPtr();
  init();
}

void SqlBufferBase::drivePack()
{
  if (NOT packed())
    pack();

  setPacked(TRUE);
}

void SqlBufferBase::driveUnpack()
{
  if (packed())
    {
      fixupVTblPtr();
      unpack();
    }

  setPacked(FALSE);
}

NABoolean SqlBufferBase::driveVerify(ULng32 maxBytes)
{
  fixupVTblPtr();
  return verify(maxBytes);
}

void SqlBufferBase::fixupVTblPtr()
{
  switch (bufType())
    {
#ifndef UDRSERV_BUILD
    case OLT_:
      {
	SqlBufferOlt sb;
	str_cpy_all ((char *)this, (char*)&sb, sizeof(char *));
      }
    break;
#endif // UDRSERV_BUILD

    case DENSE_:
      {
	SqlBufferDense sb;
	str_cpy_all ((char *)this, (char*)&sb, sizeof(char *));
      }
    break;

    case NORMAL_:
      {
	SqlBufferNormal sb;
	str_cpy_all ((char *)this, (char*)&sb, sizeof(char *));
      }
    break;
    
    default:
      {
      }
    break;

    }
}

void SqlBufferBase::pack()
{
}

void SqlBufferBase::unpack()
{
}

NABoolean SqlBufferBase::verify(ULng32 maxBytes) const
{
  return TRUE;
}

// positions to the first tupp descriptor in the buffer.
void SqlBufferBase::position()
{
}

void * SqlBufferBase::operator new(size_t size, char*s)
{
  return s; 
}

SqlBufferBase::moveStatus 
  SqlBufferBase::moveInSendOrReplyData(NABoolean isSend,   // TRUE = send
				       NABoolean doMoveControl, // TRUE = move 
				       // control always
				       NABoolean doMoveData, // TRUE = move data.
				       void * currQState,  // up_state(reply) or
				       // down_state(send)
				       ULng32 controlInfoLen,
				       ControlInfo ** controlInfo,
				       ULng32 projRowLen,
				       tupp_descriptor ** outTdesc,
				       ComDiagsArea * diagsArea,
				       tupp_descriptor ** diagsDesc,
				       ex_expr_base * expr, 
				       atp_struct * atp1,
				       atp_struct * workAtp, 
				       atp_struct * destAtp, 
				       unsigned short tuppIndex,
				       NABoolean doMoveStats, 
				       ExStatisticsArea * statsArea,
				       tupp_descriptor ** statsDesc,
				       NABoolean useExternalDA,
				       NABoolean callerHasExternalDA,
				       tupp_descriptor * defragTd
#if (defined(_DEBUG) )
				       ,ex_tcb * tcb
#endif
				       ,NABoolean noMoveWarnings
  )
{
  return MOVE_ERROR;
}

NABoolean SqlBufferBase::moveOutSendOrReplyData(NABoolean isSend,
						void * currQState,
						tupp &outTupp,
						ControlInfo ** controlInfo,
						ComDiagsArea ** diagsArea,
						ExStatisticsArea ** statsArea,
						Int64 * numStatsBytes,
						NABoolean noStateChange,
						NABoolean unpackDA,
						CollHeap * heap)
{
  return FALSE;
}

// positions to the "tupp_desc_num"th tupp descriptor.
void SqlBufferBase::position(Lng32 ){};

// returns the 'next' tupp descriptor. Increments the
// position. Returns null, if none found.
tupp_descriptor * SqlBufferBase::getNext(){return NULL;};

// returns the 'current' tupp descriptor. Does not increment
// position. Returns null, if none found.
tupp_descriptor * SqlBufferBase::getCurr(){return NULL;};

// advances the current tupp descriptor
void SqlBufferBase::advance(){};


// add a new tuple descriptor to the end of the buffer
tupp_descriptor *SqlBufferBase::add_tuple_desc(Lng32 tup_data_size)
{
  return NULL;
}

// remove the tuple desc with the highest number from the buffer
void SqlBufferBase::remove_tuple_desc()
{
}

////////////////////////////////////////////////////////////////////////////
// class SqlBuffer
////////////////////////////////////////////////////////////////////////////
void SqlBuffer::init(ULng32 in_size_in_bytes,
		      NABoolean clear)
{
  SqlBufferBase::init(in_size_in_bytes, clear);

  Lng32 sb_size      = getClassSize(); 
  freeSpace_        = sizeInBytes_ - ROUND8(sb_size); 
  bufferStatus_     = EMPTY;
  maxTuppDesc_      = 0;
  tuppDescIndex_    = 0;
  srFlags_          = DEFAULT_;
  flags_            = 0;

  setPacked(FALSE);
}

void SqlBuffer::init()
{
  Lng32 sb_size     = getClassSize();

  SqlBufferBase::init();

  freeSpace_        = sizeInBytes_ - ROUND8(sb_size); 
  bufferStatus_     = EMPTY;
  maxTuppDesc_      = 0;
  tuppDescIndex_    = 0;
  srFlags_          = DEFAULT_;
  flags_            = 0;

  setPacked(FALSE);
}

// RETURNS: -1 if this buffer is free and reinitialized, 0 if not.
short SqlBuffer::freeBuffer()
{
  // First check if the buffer is in use. If it is in use, do not call
  // isFree() or any other virtual method, as the vptr may not be valid,
  // i.e., the object may be in a packed state.
  if (bufferStatus_ == IN_USE)
    return 0;

  static SqlBufferOlt    sv_sb_olt;
  static SqlBufferDense  sv_sb_dense;
  static SqlBufferNormal sv_sb_normal;
  static SqlBufferOltSmall sv_sb_olt_small;

  if ((memcmp(this, &sv_sb_normal, sizeof(char *)) == 0) ||
      (memcmp(this, &sv_sb_dense, sizeof(char *)) == 0) ||
      (memcmp(this, &sv_sb_olt_small, sizeof(char *)) == 0) ||
      (memcmp(this, &sv_sb_olt, sizeof(char *)) == 0)) {
    ;
  }
  else {
    printf("SQ: Buffer virtual function pointer not yet set\n");fflush(stdout);
    return 0;
  }

  short freeState = isFree();
  if (freeState && (bufferStatus_ != EMPTY) )
    init(sizeInBytes_);

  return freeState;
} 

void SqlBuffer::compactBuffer()
{
}
 
tupp_descriptor * SqlBuffer::getPrev(){return NULL;};


short SqlBuffer::space_available(Lng32 tup_data_size)
{
  if (freeSpace_ >= (Lng32)(tup_data_size + sizeof(tupp_descriptor)))
    return -1;
  else
    return 0;
};


tupp_descriptor * SqlBuffer::getTuppDescriptor(Lng32 tupp_desc_num)
{
  return NULL;
}

// print buffer info. For debugging
void SqlBuffer::printInfo() {
  cerr << this << " Status: " << bufferStatus_
    << " maxTuppDesc: " << maxTuppDesc_
    << " freeSpace: " << freeSpace_;
  Lng32 refcount = 0;
  Lng32 firstref = -1;
  for (Int32 i = 0; (i < maxTuppDesc_); i++)
    if (tupleDesc(i)->getReferenceCount()) {
      if (firstref == -1)
	firstref = i;
      refcount++;
    };
  cerr << " ref tupps: " << refcount << " first: " << firstref << endl;
};

////////////////////////////////////////////////////////////////////
// Moves control info to send(input) or reply buffer, if needed.
// Moves control info if previous state is different than the
// current state. As long as state doesn't change, all tuples
// in the send/reply buffer are data tupps. A new control tupp is
// added when state changed. 
// Allocates space for data, if needed (projRowLen > 0). Returns
// pointer to allocated tupp_descriptor.
// If diagsLen is greater than 0, allocates space for diags area.
// The tupp_descriptor contains info
// indicating whether it is a control or data tupp.
// Returns TRUE if buffer is full, FALSE otherwise.
////////////////////////////////////////////////////////////////////
SqlBuffer::moveStatus SqlBuffer::moveInSendOrReplyData(NABoolean isSend,
					    NABoolean doMoveControl,
					    NABoolean doMoveData,
					    void * currQState,
					    ULng32 controlInfoLen,
					    ControlInfo ** controlInfo,
					    ULng32 projRowLen,
					    tupp_descriptor ** outTdesc,
					    ComDiagsArea * diagsArea,
					    tupp_descriptor ** diagsDesc,
					    ex_expr_base * expr, 
					    atp_struct * atp1,
					    atp_struct * workAtp, 
                                            atp_struct * destAtp,
					    unsigned short tuppIndex,
					    NABoolean doMoveStats, 
			                    ExStatisticsArea * statsArea,
					    tupp_descriptor ** statsDesc,
                                            NABoolean useExternalDA,
                                            NABoolean callerHasExternalDA,
                                            tupp_descriptor * defragTd
#if (defined(_DEBUG) )
                                            ,ex_tcb * tcb
#endif
					    ,NABoolean noMoveWarnings
                                            )
{
  // if this is the first record to be moved in,
  // or current state is different than previous 
  // state, then move in control info first.
  tupp_descriptor * cdesc = NULL;
  tupp_descriptor * tdesc = NULL;
  NABoolean controlInfoMoved = FALSE;
  Lng32 daMark = -1;
  Lng32 atpDaMark = -1;
  SqlBuffer::moveStatus retcode = MOVE_SUCCESS;
  NABoolean moveControlInfo = FALSE;
  NABoolean setUpStatusToError = FALSE;

  if ((srFlags() == REPLY_GET_UNIQUE_REQUEST) &&
      (NOT doMoveData))
    {
      doMoveData = TRUE;
      projRowLen = 0;
    }
  
  if ((getTotalTuppDescs() == 0)                                     ||
      (diagsArea != NULL)                                            ||
      ((isSend == TRUE) &&  (NOT (srControlInfo_.getDownState() == 
				  *((down_state *)currQState)))))
    {
      moveControlInfo = TRUE;
    }

  if (moveControlInfo == FALSE)
    {
      if (srFlags() == REPLY_GET_UNIQUE_REQUEST)
	{
	  if ((((up_state *)currQState)->parentIndex -
	       srControlInfo_.getUpState().parentIndex) == 1)
	    {
	      if (NOT doMoveData)
		{
		  doMoveData = TRUE;
		  projRowLen = 0;
		}
	    }
	  else if (((up_state *)currQState)->parentIndex !=
		   srControlInfo_.getUpState().parentIndex) 
	    {
	      moveControlInfo = TRUE;
	    }
	  else
	    {
	      doMoveData = FALSE;
	    }
	}
      else
	{
	  if ((doMoveControl) ||
	      ((isSend == FALSE) && (NOT (srControlInfo_.getUpState() == 
					  *((up_state *)currQState)))))
	    moveControlInfo = TRUE;

	}
    }

  NABoolean prevHadExternalDA = flags_ & CURR_HAS_EDA;

  if (callerHasExternalDA)
    {
      // External DiagsAreas always need a ControlInfo so that 
      // moveOutSendOrReplyData will notice them.
      moveControlInfo = TRUE;
      flags_ |= CURR_HAS_EDA;
    }
  else if (!callerHasExternalDA && prevHadExternalDA)
    {
      // If previous entry in this buffer had an External DiagsAreas 
      // but this one does not, make sure there is a ControlInfo so that 
      // moveOutSendOrReplyData can reset srControlInfo_.flags_
      moveControlInfo = TRUE;
      flags_ &= ~CURR_HAS_EDA;
    }
  else
    {
      // Have not changed state of SqlBuffer WRT whether curr entry has 
      // an external diags area.  Hence, no need to force a ControlInfo 
      // to note absence of external DA.
    }

  if (moveControlInfo)
    {
      if (controlInfoLen > 0)
	{
	  cdesc = add_tuple_desc(controlInfoLen);
	  
	  if (!cdesc)
	    {
	      return SqlBuffer::BUFFER_FULL; // buffer is full
	    }
      
	  // mark that this is a control tupp descriptor
	  cdesc->setControlTupleType();

	  controlInfoMoved = TRUE;
	  ControlInfo *ci;
      
          ci = (ControlInfo *)(cdesc->getTupleAddress());

          ci->resetFlags(); // no constructor got called for ci
          ci->setIsExtDiagsAreaPresent(callerHasExternalDA);
      
          if (currQState)
	    {
	      if (isSend == TRUE)
	        {
                  ci->getDownState() = *(down_state *)currQState;
	        }
	      else
	        {
                  ci->getUpState() = *(up_state *)currQState;
	        }

	      if (controlInfo)
	        *controlInfo = ci;
	    }
	} // controlInfoLen > 0
    }

  // now move data. Note that if space for data is to be allocated,
  // and the previous tupp moved was control, then this data MUST
  // be moved in the same reply buffer. In other words, the last
  // tuple in a reply buffer cannot be a control, if that control
  // has associated data.

  UInt32 defragLength = 0;
  UInt32 rowLen = projRowLen;
  UInt32 *rowLenPtr = &rowLen;
#ifndef UDRSERV_BUILD
  ex_expr::exp_return_type expr_retcode = ex_expr::EXPR_OK;
#endif
  if (doMoveData == TRUE)
    {
#ifndef UDRSERV_BUILD
    if (defragTd &&
        projRowLen > 0 &&
        SqlBufferGetTuppSize(projRowLen, bufType()) > (ULng32)getFreeSpace() )
    { // if we are here then there is not enough space to hold the max row size.
      // we apply the expression in the defrag buffer and get the actual length
      // which may be smaller then the max row size and may fit in the remaining
      // buffer space
      destAtp->getTupp(tuppIndex) = defragTd;
      defragTd->setReferenceCount(1);

      expr_retcode = expr->eval(atp1, workAtp, 0, -1, rowLenPtr);
      // if no errors then we allocate the actual size. If errors occur then we do not
      // allocate and skip the defragmentation for this row
      if (expr_retcode != ex_expr::EXPR_ERROR)
      {
        defragLength = rowLen;

#if (defined(_DEBUG) )
        char txt[] = "exchange";
        sql_buffer_pool::logDefragInfo(txt,
                        SqlBufferGetTuppSize(projRowLen, bufType()),
                        SqlBufferGetTuppSize(rowLen, bufType()),
                        getFreeSpace(),
                        this,
                        this->getTotalTuppDescs(),
                        tcb);
#endif
      }
    }

#endif

    tdesc = add_tuple_desc(rowLen);



      if (! tdesc)
	{
	  // no space to move data. Release the control
	  // information, if moved, from the reply buffer.

	  if (controlInfoMoved == TRUE)
	    remove_tuple_desc();

          return SqlBuffer::BUFFER_FULL;
	}
      
      // mark that this is a data tupp descriptor
      tdesc->setDataTupleType();

      if (outTdesc != NULL)
	*outTdesc = tdesc;
    }

#ifndef UDRSERV_BUILD
  if ((expr != NULL) && (tdesc != NULL))
    {
      destAtp->getTupp(tuppIndex) = tdesc;
      if (diagsArea != NULL)
	daMark = diagsArea->mark();

      NABoolean backoutAndRestart = FALSE;
      NABoolean atp1HadNoDiagsAreaBefore = TRUE;
      if (atp1->getDiagsArea() != NULL)
        {
	  atpDaMark = atp1->getDiagsArea()->mark();
          atp1HadNoDiagsAreaBefore = FALSE;
        }
    
      //Clear the last 4 bytes of row buffer. This is to ensure the last 4
      //filler bytes are initialized to zero. Also check to make sure the 
      //rowlength is >= 4 before clearing the bytes.
      if(rowLen >= 4) {
        char* fillerBytes = (char*)(tdesc->getTupleAddress() + (rowLen -4));
        fillerBytes[0] = 0;
        fillerBytes[1] = 0;
        fillerBytes[2] = 0;
        fillerBytes[3] = 0;
      }

    if (defragLength != 0)
    {
      str_cpy_all(tdesc->getTupleAddress(),
                  defragTd->getTupleAddress(),
                  defragLength);

    }
    else
    {
      // get row length after evaluating the expression.  If the rowlen is modified,
      // then this must be a CIF row and can be resized.
      // Initialize rowLen to projRowLen because if the expr evaluation
      // decides not to update the row length, there is no need to resize.

      expr_retcode = expr->eval(atp1, workAtp,
				0, -1, rowLenPtr);
    
      if (expr_retcode == ex_expr::EXPR_ERROR)
        {
          if (isSend)
            {
              // No point flowing request down further.  Back out and return the error.
              destAtp->getTupp(tuppIndex).release();
              remove_tuple_desc();

              if (controlInfoMoved == TRUE)
                remove_tuple_desc();

              if (workAtp == destAtp)
                destAtp->release();

              return MOVE_ERROR;
            }
          else 
            {
              // Errors raised while replying have to be processed here...
              if (controlInfoMoved == FALSE)
                {
                    // ... and we must have a ControlInfo to process errors.
                  backoutAndRestart = TRUE;
                }
            }
        }
      else // success
        { // resize the row if needed
          if (rowLenPtr && (*rowLenPtr != projRowLen))
            resize_tupp_desc(*rowLenPtr, tdesc->getTupleAddress());
        }
      }

      if (      // Has a new diags area appeared?
	   (atp1->getDiagsArea() != NULL) && (atp1HadNoDiagsAreaBefore) 
	   &&    // And we are sending diags areas externally?
	   (useExternalDA)
	   &&    // And state transistion from no DA to yes DA?
	   ((flags_ & CURR_HAS_EDA) == FALSE)
	   )
	{
	  backoutAndRestart = TRUE;
	  if (prevHadExternalDA)
	    {
	      flags_ |= CURR_HAS_EDA;
	    }
	  else
	    {
	      flags_ &= ~CURR_HAS_EDA;
	    }
	}
      
      // Sometimes we backout and restart, so that we guarantee a ControlInfo:
      //
      // 1. If an error condition is raised from the move expr, and we have no 
      //  ControlInfo into which we can record Q_SQLERROR;
      // 2. If a new diags area appears from the move expr, and the caller is using 
      //  external diags areas and the current tuple placed into the buffer did
      //  not have an external diags area associated with it.
      // 
      // Logic for this test is above.

      if (backoutAndRestart)
        {
          // Start over, this time ask for a new ctrl info.
          destAtp->getTupp(tuppIndex).release();
          remove_tuple_desc();

          if (controlInfoMoved == TRUE)
            remove_tuple_desc();

          atp1->getDiagsArea()->rewind(atpDaMark);

          return moveInSendOrReplyData(isSend,
                                   TRUE,  // force a new ctrl info
                                   doMoveData,
                                   currQState,
                                   controlInfoLen,
                                   controlInfo,
                                   projRowLen,
                                   outTdesc,
                                   NULL, diagsDesc,
                                   expr, atp1, workAtp,
                                   destAtp, 
                                   tuppIndex, 
				   doMoveStats,
                                   statsArea, 
                                   statsDesc, 
                                   useExternalDA, 
                                   useExternalDA, // always have ext. DA if used
                                   defragTd
#if (defined(_DEBUG) )
                                   ,tcb
#endif
				   ,noMoveWarnings
          );
        }

      
      if (expr_retcode == ex_expr::EXPR_ERROR)
        {
          if (isSend == FALSE)
            {
              ((ControlInfo *)(cdesc->getTupleAddress()))->getUpState().status
                = ex_queue::Q_SQLERROR;
	      setUpStatusToError = TRUE;
              retcode = MOVE_ERROR;
            }
        }

      
      if (diagsArea == NULL)
        {
          diagsArea = atp1->getDiagsArea();
        }

      if (workAtp == destAtp)
        destAtp->release();

    } // if ((expr != NULL) && (tdesc != NULL))

#endif // UDRSERV_BUILD
  
  // if warnings are not to be sent, dont send them.
  // this is done when a vsbb buffer is being shipped.
  // vsbb and sidetree eid code cannot handle input warnings.
  // one day we will teach it to handle them.
  if ((diagsArea != NULL && useExternalDA == FALSE) &&
      (diagsArea->getNumber(DgSqlCode::ERROR_) == 0) &&
      ((diagsArea->getNumber(DgSqlCode::WARNING_) > 0) && noMoveWarnings))
    {
      // do nothing. 
    }
  else if (diagsArea != NULL && useExternalDA == FALSE)
    {
      // allocate space for diags area.
      tupp_descriptor * tempDiagsDesc = add_tuple_desc(
                                diagsArea->packedLength());
      if (tempDiagsDesc == NULL)
	{
	  // don't have enough space in this input buffer for diags.
	  // remove the tuple data desc, if any.
	  if (tdesc != NULL)
	    remove_tuple_desc();

	  // remove the control desc.
          if (controlInfoMoved == TRUE)
	    remove_tuple_desc();

	  if (daMark >= 0)
	    diagsArea->rewind(daMark);

	  return  SqlBuffer::BUFFER_FULL;  // not enough space.
	}

      // mark that this is a diags tupp descriptor
      (tempDiagsDesc)->setDiagsTupleType();

      if (diagsDesc)
        *diagsDesc = tempDiagsDesc;
    }

#ifndef UDRSERV_BUILD
  if ((doMoveStats) && (statsArea != NULL))
    {
      // stats can only be returned with EOD. So no data should
      // be allocated at this time.
      ex_assert((tdesc == NULL), "Cannot have data with stats");

      // allocate space for stats area.
      *statsDesc = add_tuple_desc(statsArea->packedLength());
      if (*statsDesc == NULL)
	{
	  // don't have enough space in this input buffer for stats.

	  // remove the control desc.
	  remove_tuple_desc();
	
	  return  SqlBuffer::BUFFER_FULL;  // not enough space.
	}

      // mark that this is a stats tupp descriptor
      (*statsDesc)->setStatsTupleType();
    }
#endif // UDRSERV_BUILD
  
  if (controlInfoMoved && currQState)
    {
      // Set srControlInfo_ to the queue state of the most recent
      // row in the SqlBuffer. Do this last, because only at this
      // point can we be sure that the row really fits into the
      // buffer.

      if (isSend)
	srControlInfo_.getDownState() = *(down_state *)currQState;
      else
	{
	  srControlInfo_.getUpState() = *(up_state *)currQState;
	  if (setUpStatusToError)
	    srControlInfo_.getUpState().status = ex_queue::Q_SQLERROR;
	}
    }
  else if ((srFlags() == REPLY_GET_UNIQUE_REQUEST) &&
	   (doMoveData) &&
	   (NOT isSend) &&
	   (currQState))
    {
      srControlInfo_.getUpState() = *(up_state *)currQState;
    }

  return retcode;
}

/////////////////////////////////////////////////////////////////////////
// This method returns pairs of control and data(outTupp). The data
// returned may not be present for some cases, like Q_NO_DATA up state.
// The control and data inside the SqlBuffer are stored in a packed
// form while sending from/to one process to/from another. This method
// unpacks the control and data depending on the format described by
// srFlags_ field, and returns the control/data pair to caller.
//
// if noStateChange is set, then this moveOut is being called in read
// mode and has not yet been shipped. State of tuppDescs
// should not be changed in this case.
//
// See comments in sql_buffer.h too.
//
// In the following comments, 
//     C means a control tuple.  
//     D means a data tuple.
//     P means the parent index. Pn means parent index 'n'.
//
// Buffer Layout is how tuples were stored inside the buffer by
// the method moveInSendOrReplyData. A NULL data tuple stored means
// that this is a data entry of length zero. 
//
// Returned Tuple Pair is how this method returns data. A NULL data
// returned means no data was moved to outTupp.
//
// SEND_SAME_REQUEST:
//   Buffer Layout:    C1 D11 D12 D13 C2 C3 D31 D32
//                     C1, C2 and C3 differ in their downState.
//
//   Returned tuples:  (C1 D11), (C1 D12), (C1 D13), (C2 NULL), 
//                     (C3 D31), (C3 D32)
//
// SEND_CONSECUTIVE_REQUEST:
//   Buffer Layout:    C1P1 D11 D12 D13 C2P9 C3P2 D31 D32
//                     A new control tuple is moved if the difference in
//                     parent index between two control is not equal to the
//                     number of data tuples in between.
//
//   Returned tuples:  (C1P1 D11), (C1P2 D12), (C1P3 D13), (C2P9 NULL), 
//                     (C3P2 D31), (C3P3 D32)
//
//
// REPLY_GET_UNIQUE_REQUEST:
//   Buffer Layout:   C1P1 D11 D_NULL D13   C2P9  D_NULL  C3P2 D31 D32
//
//   Returned tuples:  (C1P1 D11), (C1P1_Q_NO_DATA NULL), 
//                     (C1P2_Q_NO_DATA NULL), 
//                     (C1P3 D13), (C1P3_Q_NO_DATA NULL),
//                     (C2P9_Q_NO_DATA NULL), 
//                     (C3P2 D31), (C3P2_Q_NO_DATA NULL),
//                     (C3P3 D32), (C3P2_Q_NO_DATA NULL)
//
//
// REPLY_GET_SUBSET_REQUEST:
//   Buffer Layout:    C1 D11 D12 D13 C2 C3 D31 D32
//                     C1, C2 and C3 differ in their downState.
//   Returned tuples:  (C1M1 D11), (C1M2 D12), (C1M3 D13),
//                     (C2 NULL), 
//                     (C3M1 D31), (C3M2 D32)
//
//
// REPLY_VSBB_INSERT_REQUEST:
// In the following comments, 
//     N means a control tuple, standing for a Q_NO_DATA
//     E means a control tuple, standing for a Q_SQLERROR
//     D means a data tuple
//     C means a diags area
//     M means a matchNo, associated with a control tuple.
//   In the following example, VSBB Insert was asked to insert 7 rows.
//   Buffer Layout:    N1M7   N2M0
//   Returned Tuples: (N1M1), (N2M1), (N3M1), (N4M1), (N5M1), (N6M1), 
//                    (N7M1), (N8M0)
//   The astute reader may wonder about the presence of the extra (8th) 
//   Q_NO_DATA.  This is indeed present, probably to respond to the GET_EOD
//   that was sent by partition access.
//   
//   Q_SQLERROR control tuples use their matchNo member to generate Q_NO_DATA 
//   returned tuples.  In this way, the Q_SQLERROR is returned in proper order.
//   In the following example, VSBB Insert was asked to insert 7 rows,
//   but an error occurred on the 4th.
//   Buffer Layout:   E1M3  D1 C1 N2M4   N2M0 
//   Returned Tuples: (N1M1), (N2M1), (N3M1), (E1M0 DATA DIAGS), 
//                    (N4M1), (N5M1), (N6M1), (N7M1), (N8M0)
//
/////////////////////////////////////////////////////////////////////////

NABoolean SqlBuffer::moveOutSendOrReplyData(NABoolean isSend,
					    void * currQState,
					    tupp &outTupp,
					    ControlInfo ** controlInfo,
					    ComDiagsArea ** diagsArea,
					    ExStatisticsArea ** statsArea,
					    Int64 * numStatsBytes,
					    NABoolean noStateChange,
					    NABoolean unpackDA,
					    CollHeap * heap)
{
  if (atEOTD())
    return TRUE;
  
  // Locate consecutive tupps.    
  tupp_descriptor *dataTuppDesc = 0;
  tupp_descriptor *diagsTuppDesc = 0;
  tupp_descriptor *statsTuppDesc = 0;
  tupp_descriptor *tuppDesc = 0;
  unsigned short dataDescIndex = 0;
  TupleDescInfo *dataTuppDescInfo = 0;

  ComDiagsArea * unpackedDiagsArea = NULL;

  if (tuppDescIndex_ < maxTuppDesc_)
    {
      switch (bufType())
	{
	case NORMAL_:
	  {
	    tuppDesc = tupleDesc(tuppDescIndex_);
	    
	    if (tuppDesc->isControlTuple())
	      {
		if (flags_ & IGNORE_CONTROL_FLAG) 
		  break;  // case done and break out.
		if (setupSrControlInfo( isSend, tuppDesc )
                   == DEFER_MORE_TUPPS)
                   break;
		if (NOT noStateChange)
		  tuppDesc->resetCommFlags();
		if (++tuppDescIndex_ < maxTuppDesc_)
		  tuppDesc = tupleDesc(tuppDescIndex_);
		else
		  break;  // tupp is control tuple.
	      }
	    if (tuppDesc->isDataTuple())
	      {
		dataTuppDesc = tuppDesc;
		dataDescIndex = tuppDescIndex_;
		if (++tuppDescIndex_ < maxTuppDesc_)
		  tuppDesc = tupleDesc(tuppDescIndex_);
		else
		  break;  // tupp is data tuple.
	      }
	    if (tuppDesc->isDiagsTuple())
	      {  
		if (NOT noStateChange)
		  tuppDesc->resetCommFlags();
		diagsTuppDesc = tuppDesc;
		if (++tuppDescIndex_ < maxTuppDesc_) 
		  tuppDesc = tupleDesc(tuppDescIndex_); 
		else
		  break;  // tupp is diags tuple.
	      }
	    if (tuppDesc->isStatsTuple())
	      {  
		if (NOT noStateChange)
		  tuppDesc->resetCommFlags();
		statsTuppDesc = tuppDesc;
		++tuppDescIndex_;
	      }
	    break;
	  } // case NORMAL_
	
	case DENSE_:
	  {
            SqlBufferDense* denseBuf = (SqlBufferDense*)(this);
	    tuppDesc = denseBuf->getCurrDense(); 
	    if (tuppDesc && tuppDesc->isControlTuple())
	      {
		if (flags_ & IGNORE_CONTROL_FLAG) 
		  break;  // case done and break out.
		if (setupSrControlInfo( isSend, tuppDesc )
                   == DEFER_MORE_TUPPS)
                   break;
		if (NOT noStateChange)
		  tuppDesc->resetCommFlags();
		
		denseBuf->advanceDense();
		tuppDesc = denseBuf->getCurrDense();
	      }
	    
	    if (tuppDesc && tuppDesc->isDataTuple())
	      {
		dataTuppDesc = tuppDesc;
		dataDescIndex = tuppDescIndex_;
		dataTuppDescInfo = castToSqlBufferDense()->currTupleDesc();
		
		denseBuf->advanceDense();
		tuppDesc = denseBuf->getCurrDense();
	      }
	    
	    if (tuppDesc && tuppDesc->isDiagsTuple())
	      {  
		if (NOT noStateChange)
		  tuppDesc->resetCommFlags();
		diagsTuppDesc = tuppDesc;
		
		denseBuf->advanceDense();
		tuppDesc = denseBuf->getCurrDense();
	      }
	    
	    if (tuppDesc && tuppDesc->isStatsTuple())
	      {  
		if (NOT noStateChange)
		  tuppDesc->resetCommFlags();
		statsTuppDesc = tuppDesc;
		
		denseBuf->advanceDense();
		tuppDesc = denseBuf->getCurrDense();
	      }
	    
	    break;
	  } // case DENSE_
	
	default:
	  UdrExeAssert(0, "SqlBuffer::moveOutSendOrReplyData() invalid case");
	} // switch
    } // more tupp descs
  
  if (controlInfo)
    {
      *controlInfo = &srControlInfo_;
    }
  
  srControlInfo_.setIsDataRowPresent(FALSE);
  
  if (isSend == TRUE)
    *(down_state *)currQState = srControlInfo_.getDownState();
  else
    *(up_state *)currQState = srControlInfo_.getUpState();
  
  switch (srFlags_)
    {
    case DEFAULT_:
    case SEND_SAME_REQUEST:
      {
	// if next tuple (if present) is data, return it. And advance
	// position.
	if (dataTuppDesc)
	  {
	    srControlInfo_.setIsDataRowPresent(TRUE);
	    
	    outTupp = dataTuppDesc;
	  }
      }
    break;
    
    case SEND_CONSECUTIVE_REQUEST:
      {
	if (dataTuppDesc)
	  {
	    srControlInfo_.setIsDataRowPresent(TRUE);
	    
	    outTupp = dataTuppDesc;
	    
	    // advance the parent index
	    srControlInfo_.getDownState().parentIndex++;
	  }
      }
    break;
    
    case SEND_VSBB_WITH_EOD:
      {
	// if next tuple (if present) is data, return it. And advance
	// position.
	if (dataTuppDesc)
	  {
	    srControlInfo_.setIsDataRowPresent(TRUE);
	    
	    outTupp = dataTuppDesc;

	    // need to send an EOD on the next call. 
	    flags_ |= IGNORE_CONTROL_FLAG;
	  }
	else
	  {
	    ((down_state *)currQState)->request = ex_queue::GET_EOD;
	    flags_ &= ~IGNORE_CONTROL_FLAG;
	  }
      }
    break;
    
    case REPLY_GET_UNIQUE_REQUEST:
      {
	if (dataTuppDesc)
	  {
	    if (srControlInfo_.getUpState().status == ex_queue::Q_OK_MMORE)
	      {
		srControlInfo_.setIsDataRowPresent(TRUE);
		
		outTupp = dataTuppDesc;
		// Restore dataDescIndex, dataTuppDesc.
		// Start at the same data tupple on the next call.
		// The elseif part will be executed on the next call.
		tuppDescIndex_ = dataDescIndex;
		if (castToSqlBufferDense())  // DENSE_ buffer
		  castToSqlBufferDense()->currTupleDesc() = 
		    dataTuppDescInfo;
		statsTuppDesc = diagsTuppDesc = 0;
		
		srControlInfo_.getUpState().status = ex_queue::Q_NO_DATA;
		
		// We need to return two control info for each data row.
		// Ignore next tuple on next call to this proc.
		flags_ |= IGNORE_CONTROL_FLAG;
	      }
	    else if (srControlInfo_.getUpState().status == 
		     ex_queue::Q_NO_DATA)
	      {
		srControlInfo_.getUpState().parentIndex++;
		
		srControlInfo_.getUpState().status = ex_queue::Q_OK_MMORE;
		
		flags_ &= ~IGNORE_CONTROL_FLAG;
	      }
	  }
      }
    break;
    
    case REPLY_GET_SUBSET_REQUEST:
      {
	if (dataTuppDesc)
	  {
	    srControlInfo_.setIsDataRowPresent(TRUE);
	    
	    outTupp = dataTuppDesc;
	    
	    // advance the match no
            if (srControlInfo_.getUpState().getMatchNo() != OverflowedMatchNo)
              {
                Int64 matchNo64 = srControlInfo_.getUpState().getMatchNo();
	        srControlInfo_.getUpState().setMatchNo(matchNo64 + 1);
              }
	  }
      }
    break;
    
    case REPLY_VSBB_INSERT_REQUEST:
      {
	// if next tuple (if present) is data, return it. 
	if (dataTuppDesc)
	  {
	    srControlInfo_.setIsDataRowPresent(TRUE);
	    
	    outTupp = dataTuppDesc;
	  }

	if ((diagsTuppDesc) &&
	    (unpackDA) &&
	    (heap))
	  {
	    ComDiagsArea * d = 
	      (ComDiagsArea *)(diagsTuppDesc->getTupleAddress());
	    unpackedDiagsArea = ComDiagsArea::allocate(heap);
	    unpackedDiagsArea->unpackObj(d->getType(),
					 d->getVersion(),
					 TRUE,      // sameEndianness,
					 0, // dummy for now...
					 (IpcConstMessageBufferPtr) d);
	    
	    if (unpackedDiagsArea->containsRowCountFromEID())
	      {
		flags_ |= RETURN_ROWCOUNT;
	      }

	    if (controlInfo)
	      {
		(*controlInfo)->setIsDiagsAreaUnpacked(TRUE);
	      }
	  }

	if (srControlInfo_.getUpState().matchNo > 0)
	  {
	    if (flags_ & RETURN_ROWCOUNT)
	      {
		if (flags_ & ROWCOUNT_RETURNED)
		  ((up_state *)currQState)->matchNo = 0;
		else
		  {
		    ((up_state *)currQState)->matchNo =
		      (Lng32)unpackedDiagsArea->getRowCount();
		    unpackedDiagsArea->setContainsRowCountFromEID(FALSE);
		    unpackedDiagsArea->setRowCount(0);
		    flags_ |= ROWCOUNT_RETURNED;
		  }
	      }
	    else
	      ((up_state *)currQState)->matchNo = 1; 
	    ((up_state *)currQState)->status = ex_queue::Q_NO_DATA;
	  }
	else
	  {
	    ((up_state *)currQState)->matchNo = 0; 
	  }
	
	srControlInfo_.getUpState().downIndex++;

	// We need to return matchNo control info tuples.
	// Ignore next tuple on next call to this proc.
	// It will be advanced after returning all control tuples.
	flags_ |= IGNORE_CONTROL_FLAG;
	
	if (srControlInfo_.getUpState().downIndex >= 
	    (queue_index) srControlInfo_.getUpState().matchNo)
	  {
	    flags_ &= ~IGNORE_CONTROL_FLAG;
	  }
      }
    break;
    
    } // switch on srFlags_
  
  // if next tuple (if present) is diags, return it. 
  if (diagsTuppDesc)
    {
      srControlInfo_.setIsDiagsAreaPresent(TRUE);
      
      if (unpackedDiagsArea)
	{
	  *diagsArea = unpackedDiagsArea;
	}
      else
	*diagsArea = (ComDiagsArea *)(diagsTuppDesc->getTupleAddress());
    }
  else
    {
      srControlInfo_.setIsDiagsAreaPresent(FALSE);
      if (diagsArea)
	*diagsArea = NULL;
    }
  
  // if next tuple (if present) is stats, return it. 
  if (statsArea != NULL)
    {
      if (statsTuppDesc)
	{
	  *statsArea = (ExStatisticsArea *)(statsTuppDesc->getTupleAddress());
	  *numStatsBytes = sizeof(*statsTuppDesc) + statsTuppDesc->getAllocatedSize();
	}
      else
	{
	  *statsArea = NULL;
	  *numStatsBytes = 0;
	}
    }
  
  return FALSE;
}

Int32 SqlBuffer::setupSrControlInfo( NABoolean isSend, tupp_descriptor *tuppDesc )
{
  // remember that flags_ were overloaded with relocatedAddress
  // in the tupp_descriptor (just trying to save some space).
  // Now that the buffer has reached its destination, the
  // flags_ field could not be used.
  
  ControlInfo *ci = (ControlInfo *)(tuppDesc->getTupleAddress());
  
  srControlInfo_.copyFlags(*ci);
  
  if (isSend == TRUE)
    {
      srControlInfo_.getDownState() = ci->getDownState();
    }
  else
    {
      srControlInfo_.getUpState() = ci->getUpState();
    }
  
  if (srFlags_ == REPLY_VSBB_INSERT_REQUEST)
    {
      // Use downIndex to keep track of how many tuples have been
      // returned.
      srControlInfo_.getUpState().downIndex = 0;
      // SQLERROR reply implies fatal error and OK_MMORE reply implies 
      // nonfatal error for VSBB insert
      if ((srControlInfo_.getUpState().status == ex_queue::Q_SQLERROR ||
	   srControlInfo_.getUpState().status == ex_queue::Q_OK_MMORE ) 
	   && srControlInfo_.getUpState().matchNo > 0)
      {
          // Will eventually process this ci again, so mark it
          // so that only one sequence of Q_NO_DATAs are unpacked.
          ci->getUpState().matchNo = 0;
          return DEFER_MORE_TUPPS;
      }
    }
  return PROCEED_MORE_TUPPS;
}


NABoolean SqlBuffer::findAndCancel( queue_index pindex, NABoolean startFromBeginning)
{
  // start at the beginning or at curr, depending on argument.
  // for each tuple, see if it is a control tuple.
  // if so, see if downstate.parentIndex matches pindex.
  // if so, change downstate.request to GET_NOMORE & return TRUE, 
  // if not found, return FALSE.
  // save and restore tuppDescIndex_, regardless of outcome.
  Lng32 saveTuppDescIndex = tuppDescIndex_;
  NABoolean wasFound = FALSE;

  if (startFromBeginning)
    position();

  while ( ! atEOTD() )
    {
      if (getCurr()->isControlTuple())
        {
          down_state *ds = &((ControlInfo *)(getCurr()->getTupleAddress()))->
                              getDownState();
          if (ds->parentIndex == pindex)
            {
              // eureka
              ds->request = ex_queue::GET_NOMORE;
              wasFound = TRUE;
              break;
            }
        }
      advance();
    }
  position(saveTuppDescIndex);
  return wasFound;
}

unsigned short SqlBuffer::getNumInvalidTuplesUptoNow()
  { 
    UdrExeAssert(0, "Do not call the base class method");
    return 0;
  }
    
void SqlBuffer::setNumInvalidTuplesUptoNow(Lng32 val)
  {
    UdrExeAssert(0, "Do not call the base class method");
  }

void SqlBuffer::incNumInvalidTuplesUptoNow()
  { 
    UdrExeAssert(0, "Do not call the base class method");
  }

void SqlBuffer::decNumInvalidTuplesUptoNow()
  { 
    UdrExeAssert(0, "Do not call the base class method");
  }

void SqlBuffer::advanceToNextValidTuple()
{
  advance();
  while ((!(atEOTD())) && getCurr()->isInvalidTuple()) 
  {
    advance();
    incNumInvalidTuplesUptoNow();	    
  }
}


static Int64 debugCnt = 0;

void SqlBuffer::setSignature()
  {
  signature_ = ++debugCnt;
  }

NABoolean SqlBuffer::checkSignature(const Int32 nid, const Int32 pid,
                                    Int64 *signature, const Int32 action)
  {
  if (signature_ <= *signature)
    {
      if (action > 0)
        {
          char coreFile[512];
          msg_mon_dump_process_id(NULL, nid, pid, coreFile);
        }
      return FALSE;
    }
  *signature = signature_;
  return TRUE;
  }


////////////////////////////////////////////////////////////////////////////
// class SqlBuffer
////////////////////////////////////////////////////////////////////////////
void SqlBufferNormal::init(ULng32 in_size_in_bytes,
		     NABoolean clear)
{
  SqlBuffer::init(in_size_in_bytes, clear);

  dataOffset_ = ROUND8(sizeof(*this)); // data starts after this object

  // last byte in buffer, rounded to previous 4 byte boundary
  tupleDescOffset_ = ROUND4(sizeInBytes_ - 3);   

  numInvalidTuplesUptoCurrentPosition_  = (unsigned short) NO_INVALID_TUPLES;

  if (clear)
    memset((char*)this+dataOffset_, 0, freeSpace_);

  // adjust freeSpace_ to account for the unused bytes due to rounding.
  freeSpace_ = freeSpace_ - (sizeInBytes_ - tupleDescOffset_);

  normalBufFlags_ = 0;
}

void SqlBufferNormal::init()
{
  SqlBuffer::init();

  dataOffset_ = ROUND8(sizeof(*this)); // data starts after this object

  // last byte in buffer, rounded to previous 4 byte boundary
  tupleDescOffset_ = ROUND4(sizeInBytes_ - 3);                                                

  numInvalidTuplesUptoCurrentPosition_  = (unsigned short) NO_INVALID_TUPLES;

  // adjust freeSpace_ to account for the unused bytes due to rounding.
  freeSpace_ = freeSpace_ - (sizeInBytes_ - tupleDescOffset_);

  normalBufFlags_ = 0;
}


// allocate a new tupp descriptor
tupp_descriptor *SqlBufferNormal::allocate_tuple_desc(Lng32 tup_data_size)
{
  // NOTE: SqlBufferNormal::allocate_tuple_desc and
  // SqlBufferNormal::remove_tuple_desc must be kept
  // consistent. remove_tuple_desc should correctly reverse any
  // modifications done by the most recent allocate_tuple_desc.

  UInt32 rounded_size = ROUND8(tup_data_size);
  Lng32 freeSpaceNeeded = rounded_size + sizeof(tupp_descriptor);
  
  if (freeSpace_ < freeSpaceNeeded) // no free space to allocate this tuple
    return NULL;

  freeSpace_ -= freeSpaceNeeded;
  tupp_descriptor * td = tupleDesc(maxTuppDesc_);
  maxTuppDesc_++;
  
  td->init((ULng32)tup_data_size,
	   0, 
	   &((char *)this)[dataOffset_]);

  dataOffset_ += rounded_size;

  // if buffer is empty, then change status to partially full.
  if (bufferStatus_ == EMPTY)
    bufferStatus_ = PARTIAL;

  return td;
}  

// adds a tuple descriptor
tupp_descriptor *SqlBufferNormal::add_tuple_desc(Lng32 tup_data_size)
{
  return allocate_tuple_desc(tup_data_size);
}  
  
// removes the last tuple descriptor.
void SqlBufferNormal::remove_tuple_desc()
{
  // NOTE: SqlBufferNormal::allocate_tuple_desc and
  // SqlBufferNormal::remove_tuple_desc must be kept
  // consistent. remove_tuple_desc should correctly reverse any
  // modifications done by the most recent allocate_tuple_desc.

  if (maxTuppDesc_ > 0)
    {
      maxTuppDesc_--;
      tupp_descriptor * td = tupleDesc(maxTuppDesc_);
      UInt32 rounded_size = ROUND8(td->getAllocatedSize());
      dataOffset_ -= rounded_size;
      freeSpace_ += rounded_size + sizeof(tupp_descriptor);
      if (maxTuppDesc_ == 0)
        bufferStatus_ = EMPTY;
    }
}

// Resize the last allocated tuple in the Dense buffer.
void SqlBufferDense::resize_tupp_desc(UInt32 tup_data_size, char *dataPointer)
{
#ifndef UDRSERV_BUILD
  ex_assert(lastTupleDesc(), "resizing non-existant td");
#endif
  tupp_descriptor * td = lastTupleDesc()->tupleDesc();
  UInt32 rounded_new_size = ROUND8(tup_data_size);
  UInt32 rounded_old_size = ROUND8(td->getAllocatedSize());
  Int32 diff = rounded_old_size - rounded_new_size;

#ifndef UDRSERV_BUILD
  ex_assert((diff >= 0), "trying to increase tupp size"); 
  ex_assert(dataPointer == td->getTupleAddress(), "Bad resize");
#endif
  freeSpace_ += diff;

  td->setAllocatedSize(tup_data_size);
}


// adjusts data size in the last tupp descriptor
void SqlBufferNormal::resize_tupp_desc(UInt32 tup_data_size, char *dataPointer)
{
  tupp_descriptor * td = tupleDesc(maxTuppDesc_ - 1);
  UInt32 rounded_new_size = ROUND8(tup_data_size);
  UInt32 rounded_old_size = ROUND8(td->getAllocatedSize());
  Int32 diff = rounded_old_size - rounded_new_size;
#ifndef UDRSERV_BUILD
  ex_assert((diff >= 0), "trying to increase tupp size"); 
  ex_assert(dataPointer == td->getTupleAddress(), "Bad resize");
#endif
  dataOffset_ -= diff;
  freeSpace_ += diff;

  td->setAllocatedSize(tup_data_size);
}

// RETURNS: -1 if this buffer is free, 0 if not.
short SqlBufferNormal::isFree()
{
  // if this buffer is in use by an I/O operation
  // then don't free it. It has not been filled up yet.
  if (bufferStatus_ == IN_USE)
    return 0;
  
  // if the buffer is empty then it's free, of course
  if (bufferStatus_ == EMPTY)
    return -1;

  // if all tuple descriptors in this SqlBuffer are not being referenced,
  // then set the buffer status to EMPTY.
  // Backward search for efficiency.  (eric) 
  for (Int32 i = (maxTuppDesc_ - 1); i >= 0; i--)
    {
      if (tupleDesc(i)->getReferenceCount() != 0)
	// this tuple descriptor is still in use
      	return 0;
    }

  return -1;
} 

// positions to the first tuple descriptor in the buffer.
void SqlBufferNormal::position()
{
  tuppDescIndex_ = 0;
}

// positions to the "tupp_desc_num"th tupp descriptor.
void SqlBufferNormal::position(Lng32 tupp_desc_num)
{
  tuppDescIndex_ = (unsigned short)tupp_desc_num;
}

// print buffer info. For debugging
void SqlBufferNormal::printInfo() {
  cerr << this << " Status: " << bufferStatus_
    << " maxTuppDesc: " << maxTuppDesc_
    << " freeSpace: " << freeSpace_
    << " numInvalidTuplesUptoCurrentPosition:  " << numInvalidTuplesUptoCurrentPosition_ ;
  Lng32 refcount = 0;
  Lng32 firstref = -1;
  for (Int32 i = 0; (i < maxTuppDesc_); i++)
    if (tupleDesc(i)->getReferenceCount()) {
      if (firstref == -1)
	firstref = i;
      refcount++;
    };
  cerr << " ref tupps: " << refcount << " first: " << firstref << endl;
};

// returns the 'next' tuple descriptor. Increments the
// position. Returns null, if none found.
tupp_descriptor * SqlBufferNormal::getNext()
{
  if (tuppDescIndex_ < maxTuppDesc_)
    return tupleDesc(tuppDescIndex_++);
  else
    return 0;
}

// returns the 'prev' tuple descriptor. Decrements the
// position. Returns null, if none found.
tupp_descriptor * SqlBufferNormal::getPrev()
{
  if (tuppDescIndex_ > 0)
    return tupleDesc(tuppDescIndex_--);
  else
    return 0;
}

// Returns the "tupp_desc_num"th tupp descriptor. The input
// tupp_desc_num is 1-based, so the first tupp_descriptor
// is tupp_desc_num = 1.
// Returns NULL, if tupp_desc_num is greater than maxTuppDesc_.
tupp_descriptor * SqlBufferNormal::getTuppDescriptor(Lng32 tupp_desc_num)
{
  if (tupp_desc_num > maxTuppDesc_)
    return 0;
  else
    return tupleDesc(tupp_desc_num-1);
}


void SqlBufferNormal::pack()
{
  UInt32 i = 0;
  while (i < maxTuppDesc_)
    {
      tupleDesc(i)->setTupleOffset((Lng32)(
                                   (char *)tupleDesc(i)->getTupleAddress() -
				   (char *)this));
      
      i++;
    }  
}

void SqlBufferNormal::unpack()
{
  UInt32 i = 0;
  while (i < maxTuppDesc_)
    {
      tupleDesc(i)->setTupleAddress((char *)this +
				    tupleDesc(i)->getTupleOffset());
      
      i++;
    }  
}

// This verify() method will be called only on packed objects. It
// should return FALSE if it detects that unpacking the object could
// create any internal inconsistencies, or bad pointers that reference
// memory outside the bounds of this object. Currently verify() is
// only called by the ExUdrTcb class when it receives a data reply
// from the non-trusted UDR server.
NABoolean SqlBufferNormal::verify(ULng32 maxBytes) const
{
  if ((ULng32) sizeInBytes_ > maxBytes)
    return FALSE;
  if (tupleDescOffset_ < sizeof(*this))
    return FALSE;

  ULng32 bytesForTupps = (maxTuppDesc_ * sizeof(tupp_descriptor));

  if (bytesForTupps > maxBytes)
    return FALSE;

  // We are going to loop through all the tuples and keep track of how
  // far their data buffers extend. If any data buffer extends beyond
  // maxBytes or beyond sizeInBytes_ then we must return
  // FALSE. bytesSeen will represent the end of the furthest reaching
  // data buffer seen so far.
  ULng32 bytesSeen = sizeof(*this);
  tupp_descriptor *tdBase = (tupp_descriptor *)
    ((char *) this + tupleDescOffset_);

  for (Int32 i = 0; i < (Int32)maxTuppDesc_; i++)
  {
    ULng32 offset = tdBase[-(i+1)].getTupleOffset();
    if (offset > dataOffset_)
      return FALSE;
    ULng32 endOfData = offset + tdBase[-(i+1)].getAllocatedSize();
    bytesSeen = MAXOF(bytesSeen, endOfData);
  }

  if (bytesSeen > dataOffset_)
    return FALSE;
  if (bytesSeen > maxBytes)
    return FALSE;
  if (bytesSeen > (ULng32) sizeInBytes_)
    return FALSE;

  return TRUE;
}

unsigned short SqlBufferNormal::getNumInvalidTuplesUptoNow()
{
  return numInvalidTuplesUptoCurrentPosition_;
}
    
void SqlBufferNormal::setNumInvalidTuplesUptoNow(Lng32 val)
  {
    UdrExeAssert(val < USHRT_MAX, "total invalid tupps in buffer is larger than USHRT_MAX");
    numInvalidTuplesUptoCurrentPosition_ = (unsigned short) val;
  }

void SqlBufferNormal::incNumInvalidTuplesUptoNow()
  { 
    UdrExeAssert(numInvalidTuplesUptoCurrentPosition_ < USHRT_MAX, 
      "total invalid tupps in buffer is larger than USHRT_MAX");
    numInvalidTuplesUptoCurrentPosition_++;
  }

void SqlBufferNormal::decNumInvalidTuplesUptoNow()
  { 
    numInvalidTuplesUptoCurrentPosition_--;
  }

/////////////////////////////////////////////////////////////////////
// class SqlBufferDense
/////////////////////////////////////////////////////////////////////
SqlBufferDense::SqlBufferDense() : SqlBuffer(DENSE_)
{
  currTupleDesc() = NULL;
  lastTupleDesc() = NULL;
  numInvalidTuplesUptoCurrentPosition_  = (unsigned short) NO_INVALID_TUPLES;

  str_pad(filler_, 2, '\0');
}

void SqlBufferDense::init(ULng32 in_size_in_bytes,
			  NABoolean clear)
{
  SqlBuffer::init(in_size_in_bytes, FALSE);

  lastTupleDesc_ = NULL;
  currTupleDesc_ = NULL;
  numInvalidTuplesUptoCurrentPosition_  = (unsigned short) NO_INVALID_TUPLES;

  str_pad(filler_, 2, '\0');
}

void SqlBufferDense::init()
{
  SqlBuffer::init();

  lastTupleDesc_ = NULL;
  currTupleDesc_ = NULL;
  numInvalidTuplesUptoCurrentPosition_  = (unsigned short) NO_INVALID_TUPLES;

  str_pad(filler_, 2, '\0');
}

tupp_descriptor *SqlBufferDense::add_tuple_desc(Lng32 tup_data_size)
{
  ULng32 rounded_size = ROUND8(tup_data_size);
  short td_size = ROUND8(sizeof(TupleDescInfo));
  
  Lng32 freeSpaceNeeded = td_size + rounded_size;
  if (freeSpace_ < freeSpaceNeeded) // no free space to allocate this tuple
    return NULL;
  
  maxTuppDesc_ += 1;
  freeSpace_ -= freeSpaceNeeded;

  // if buffer is empty, then change status to partially full.
  if (bufferStatus_ == EMPTY)
    bufferStatus_ = PARTIAL;

  TupleDescInfo * tdi = NULL;
  if (lastTupleDesc())
    tdi = 
      (TupleDescInfo *)(lastTupleDesc()->tupleDesc()->getTupleAddress()
			+ ROUND8(lastTupleDesc()->tupleDesc()->getAllocatedSize()));
    else
      tdi = firstTupleDesc();

  tupp_descriptor * td = tdi->tupleDesc();
  td->init(tup_data_size,
	   0, 
	   (char *)td + td_size);

  setPrevTupleDesc(tdi, lastTupleDesc());
  
  if (lastTupleDesc())
    setNextTupleDesc(lastTupleDesc(), tdi);
  
  lastTupleDesc() = tdi;

  setNextTupleDesc(tdi, NULL);

  return td;
}  

tupp_descriptor *SqlBufferDense::allocate_tuple_desc(Lng32 tup_data_size)
{
  return add_tuple_desc(tup_data_size);
}

// removes the last tuple descriptor.
void SqlBufferDense::remove_tuple_desc()
{
  if (maxTuppDesc_ > 0)
    {
      // we got rid of the last tupp_descriptor
      maxTuppDesc_--;
      freeSpace_ += (ROUND8(sizeof(tupp_descriptor))
	+ ROUND8(lastTupleDesc()->tupleDesc()->getAllocatedSize()));

      if (lastTupleDesc() = getPrevTupleDesc(lastTupleDesc()))	// =assignment, NOT ==
	setNextTupleDesc(lastTupleDesc(), NULL);

      compactBuffer();
      if (maxTuppDesc_ == 0)
	bufferStatus_ = EMPTY;
    }
}

short SqlBufferDense::isFree()
{
  // if this buffer is in use by an I/O operation
  // then don't free it. It has not been filled up yet.
  if (bufferStatus_ == IN_USE)
    return 0;
  
  // if the buffer is empty then it's free, of course
  if (bufferStatus_ == EMPTY)
    return -1;

  // if all tuple descriptors in this SqlBuffer are not being referenced,
  // then set the buffer status to EMPTY.
  TupleDescInfo * tdi = lastTupleDesc();
  while (tdi)
    {
      if (tdi->tupleDesc()->getReferenceCount() != 0)
	// this tuple descriptor is still in use
      	return 0;
      tdi = getPrevTupleDesc(tdi);
    }
  
  return -1;
}

// positions to the first tuple descriptor in the buffer.
void SqlBufferDense::position()
{
  tuppDescIndex_ = 0;
  currTupleDesc() = firstTupleDesc(); 
}

void SqlBufferDense::position(Lng32 tupp_desc_num)
{
  position();
  while ((tuppDescIndex_ != tupp_desc_num) &&
         (tuppDescIndex_ < maxTuppDesc_))
    advanceDense();
}

// returns the 'next' tuple descriptor. Increments the
// position. Returns null, if none found.
tupp_descriptor * SqlBufferDense::getNext()
{
  if (tuppDescIndex_ < maxTuppDesc_)
    {
      TupleDescInfo * tdi = currTupleDesc();
      currTupleDesc() = getNextTupleDesc(tdi);
      tuppDescIndex_++;
      return tdi->tupleDesc();
    }
  else
    return NULL;
}

// returns the 'prev' tuple descriptor. Decrements the
// position. Returns null, if none found.
tupp_descriptor * SqlBufferDense::getPrev()
{
  if (tuppDescIndex_ > 0)
    {
      TupleDescInfo * tdi = currTupleDesc();
      currTupleDesc() = getPrevTupleDesc(tdi);
      tuppDescIndex_--;
      return tdi->tupleDesc();
    }
  else
    return NULL;
}

// Returns the "tupp_desc_num"th tupp descriptor. The input
// tupp_desc_num is 1-based, so the first tupp_descriptor
// is tupp_desc_num = 1.
// Returns NULL, if tupp_desc_num is greater than maxTuppDesc_.
tupp_descriptor * SqlBufferDense::getTuppDescriptor(Lng32 tupp_desc_num)
{
  if (tupp_desc_num > maxTuppDesc_)
    return 0;
  else
    return tupleDesc(tupp_desc_num-1);
}

SqlBufferHeader::moveStatus SqlBufferDense::moveInVsbbEOD(void * down_q_state)
{
  srFlags_ = SEND_VSBB_WITH_EOD;

  return MOVE_SUCCESS;
}

void TupleDescInfo::pack(Long base)
{
  tupleDesc()->setTupleOffset
    ((Long)tupleDesc()->getTupleAddress() - base);
}

void TupleDescInfo::unpack(Long base)
{
  tupleDesc()->setTupleAddress((char *)base +
			       tupleDesc()->getTupleOffset());
}

void SqlBufferDense::pack()
{
  if (maxTuppDesc_ > 0)
    {
    TupleDescInfo * temp = firstTupleDesc();
    while (temp)
      {
        TupleDescInfo * nextTDI = getNextTupleDesc(temp);
        temp->pack((Long)this);
        temp = nextTDI;
      }  
    }

  if (lastTupleDesc())
    lastTupleDesc()  = (TupleDescInfo *)((char *)lastTupleDesc()  - (char *)this);

  if (currTupleDesc())
    currTupleDesc()  = (TupleDescInfo *)((char *)currTupleDesc()  - (char *)this);
}

void SqlBufferDense::unpack()
{
  if (lastTupleDesc())
    lastTupleDesc()  = (TupleDescInfo *)((Long)lastTupleDesc()  + (char *)this);

  if (currTupleDesc())
    currTupleDesc()  = (TupleDescInfo *)((Long)currTupleDesc()  + (char *)this);

  if (maxTuppDesc_ > 0)
    {
    TupleDescInfo * temp = firstTupleDesc();
    while (temp)
      {
        temp->unpack((Long)this);
        temp = getNextTupleDesc(temp);
      }  
    }
}

unsigned short SqlBufferDense::getNumInvalidTuplesUptoNow()
{
  return numInvalidTuplesUptoCurrentPosition_;
}
    
void SqlBufferDense::setNumInvalidTuplesUptoNow(Lng32 val)
  {
    UdrExeAssert(val < USHRT_MAX, "total invalid tupps in buffer is larger than USHRT_MAX");
    numInvalidTuplesUptoCurrentPosition_ = (unsigned short) val;
  }

void SqlBufferDense::incNumInvalidTuplesUptoNow()
  { 
    UdrExeAssert(numInvalidTuplesUptoCurrentPosition_ < USHRT_MAX, 
      "total invalid tupps in buffer is larger than USHRT_MAX");
    numInvalidTuplesUptoCurrentPosition_++;
  }

void SqlBufferDense::decNumInvalidTuplesUptoNow()
  { 
    numInvalidTuplesUptoCurrentPosition_--;
  }

  #define FAILURE { DebugBreak(); }

#ifdef DO_INTEGRITY_CHECK
void SqlBuffer::integrityCheck( NABoolean ignore_omm_check)
 {
    const Lng32 savedTuppDescIndex = getProcessedTuppDescs();
    tupp p;
    ControlInfo * ci = 0;
    ComDiagsArea * d = NULL;
    ExStatisticsArea * s = NULL;
    Int64 numStatsBytes = 0;
    up_state upState;
    enum okMMoreType
    {
    NOT_KNOWN_     = 0,
    YES_           = 1,
    NO_            = 3
    } okMMoreHasData = NOT_KNOWN_;

    position();

    while (atEOTD() != -1)
      {
          moveOutSendOrReplyData(
                FALSE, // reply data
                &upState,
                p,
                &ci, &d, &s,
                &numStatsBytes,
                TRUE,  //  noStateChange
                FALSE, // unpack DA if bufInBuf
                NULL   // CollHeap (needed if unpack DA)
                );
        switch (upState.status)
          {
            case ex_queue::Q_NO_DATA:
              {
                if (ci->getIsDataRowPresent())
                  FAILURE;
                break;
              }
            case ex_queue::Q_GET_DONE:
            case ex_queue::Q_REC_SKIPPED:
              {
                if (ci->getIsDataRowPresent())
                  FAILURE;
                if (ci->getIsDiagsAreaPresent())
                  FAILURE;
                break;
              }
            case ex_queue::Q_OK_MMORE:
              {
		if (ignore_omm_check)		  
		  break;
		  
                switch (okMMoreHasData)
                  {
                    case NOT_KNOWN_:
                      {
                        okMMoreHasData = p.isAllocated() ? YES_ : NO_;
                        break;
                      }
                    case NO_:
                      {
                        if (p.isAllocated() == TRUE)
                          FAILURE;
                        break;
                      }
                    case YES_:
                      {
                        if (p.isAllocated() == FALSE)
                          FAILURE;
                        break;
                      }
                    default:
                      {
                        FAILURE;
                        break;
                      }
                  }
                  
                break;
              }
            case ex_queue::Q_STATS:
              {
                if (ci->getIsDataRowPresent())
                  FAILURE;
                if (ci->getIsDiagsAreaPresent())
                  FAILURE;
                break;
              }
            case ex_queue::Q_SQLERROR:
              {
                if (!(ci->getIsDiagsAreaPresent()))
                  FAILURE;
                break;
              }
            case ex_queue::Q_INVALID:
            default:
                FAILURE;
          }
      }
    position(savedTuppDescIndex);
    return;
  }
#else
// See sql_buffer.h for SqlBufferBase no-op implementation.
#endif

#ifndef UDRSERV_BUILD
////////////////////////////////////////////////////////////////////////////
// class sql_buffer_pool
//
//   Allocate in_number_of_buffers each with a size of buffer_size.
//   Chain them anchored at buffer_list.
////////////////////////////////////////////////////////////////////////////
sql_buffer_pool::sql_buffer_pool(Lng32 numberOfBuffers,
				 Lng32 bufferSize,
				 CollHeap * space,
				 SqlBufferBase::BufferType bufType)
  : bufType_(bufType),
    defragBuffer_(NULL),
    defragTd_(NULL)
{
  ex_assert((space != 0), "Must pass in a non-null Space pointer");
  
  currNumberOfBuffers_ = 0;
  maxNumberOfBuffers_  = numberOfBuffers;
  memoryUsed_          = 0;
  staticBufferList_    = NULL;
  dynBufferList_       = NULL;
  space_               = space;
  defaultBufferSize_   = bufferSize;

  flags_ = 0;

  staticBufferList_    = new(space_) Queue(space_);
  dynBufferList_       = new(space_) Queue(space_);

}  

sql_buffer_pool::~sql_buffer_pool()
{
  char * currBuffer;

  if (staticBufferList_)
    {
      staticBufferList_->position();
      while (currBuffer = (char *)(staticBufferList_->getCurr()))
	{
	  space_->deallocateMemory(currBuffer);
	  
	  // move to the next entry
	  staticBufferList_->advance();
	}
      
      //delete staticBufferList_;
      NADELETE(staticBufferList_, Queue, space_);
      staticBufferList_ = NULL;
    }

  if (dynBufferList_)
    {
      dynBufferList_->position();
      while (currBuffer = (char *)(dynBufferList_->getCurr()))
	{
	  space_->deallocateMemory(currBuffer);
	  
	  // move to the next entry
	  dynBufferList_->advance();
	}
      
      //delete dynBufferList_;
      NADELETE(dynBufferList_, Queue, space_);
      dynBufferList_ = NULL;
    }
  if (defragBuffer_)
  {
    space_->deallocateMemory(defragBuffer_);
  }
}

SqlBufferBase * sql_buffer_pool::addBuffer(Lng32 totalBufferSize,
					   SqlBufferBase::BufferType bufType,
					   CollHeap * space)
{
  // allocate a size that is divisible by 8, otherwise the data that
  // is allocated from the end of the buffer won't be aligned properly
  Lng32 rounded_size = ROUND8(totalBufferSize);
  
  // allocate a buffer of the right size
  char * buf = new(space) char[rounded_size];

  SqlBufferBase *newBuffer = NULL;
  if (bufType == SqlBufferBase::NORMAL_)
    newBuffer = new(buf) SqlBufferNormal();
  else if (bufType == SqlBufferBase::DENSE_)
    newBuffer = new(buf) SqlBufferDense();
  else
    newBuffer = new(buf) SqlBufferOlt();
  
  // now make the raw data buffer into an sql_buffer
  newBuffer->init(rounded_size);

  return newBuffer;
}

SqlBufferBase * sql_buffer_pool::addBuffer(Queue * bufferList,
					   Lng32 totalBufferSize,
					   SqlBufferBase::BufferType bufType,
					   bool failureIsFatal)
{
  // allocate a size that is divisible by 8, otherwise the data that
  // is allocated from the end of the buffer won't be aligned properly
  Lng32 rounded_size = ROUND8(totalBufferSize);
  
  // allocate a buffer of the right size
  char * buf = (char*)space_->allocateMemory(rounded_size, failureIsFatal);
  if (buf == NULL)
  {
    return NULL;
  }

  SqlBufferBase *newBuffer = NULL;
  if (bufType == SqlBufferBase::NORMAL_)
    newBuffer = new(buf) SqlBufferNormal();
  else if (bufType == SqlBufferBase::DENSE_)
    newBuffer = new(buf) SqlBufferDense();
  else
    newBuffer = new(buf) SqlBufferOlt();
  
  // now make the raw data buffer into an sql_buffer
  newBuffer->init(rounded_size);

  // add the newly created buffer to the end of the queue
  bufferList->insert(newBuffer);

  // update # of buffers in this pool and memory usage
  if (bufferList == dynBufferList_)
    currNumberOfBuffers_++;
  memoryUsed_ += rounded_size;

  return newBuffer;
}
// create a new buffer that is used to hold row temporarily in order
// to do defragmentation and create a tuple descriptor 
tupp_descriptor * sql_buffer_pool::addDefragTuppDescriptor(Lng32 dataSize )
{

  if (defragTd_)
  {
    return defragTd_;
  }
  if (!defragBuffer_)
  {
    Lng32 neededBufferSize =
    (Lng32) SqlBufferNeededSize( 1, dataSize );

    // allocate a size that is divisible by 8, otherwise the data that
    // is allocated from the end of the buffer won't be aligned properly
    Lng32 rounded_size = ROUND8(neededBufferSize);

    // allocate a buffer of the right size
    char * buf = (char*)space_->allocateMemory(rounded_size, FALSE);
    if (buf == NULL)
    {
      return NULL;
    }

    SqlBufferBase *newBuffer = NULL;
    if (bufType_ == SqlBufferBase::NORMAL_)
      newBuffer = new(buf) SqlBufferNormal();
    else if (bufType_ == SqlBufferBase::DENSE_)
      newBuffer = new(buf) SqlBufferDense();
    else
      newBuffer = new(buf) SqlBufferOlt();

    // now make the raw data buffer into an sql_buffer
    newBuffer->init(rounded_size);

    defragBuffer_ = newBuffer;
  }
  tupp_descriptor * td = defragBuffer_->add_tuple_desc(dataSize);

  defragTd_ = td;
  return defragTd_;
}



SqlBufferBase * sql_buffer_pool::addBuffer(Lng32 totalBufferSize,
					   bool failureIsFatal)
{
  return addBuffer(dynBufferList_, totalBufferSize, bufType_,
                   failureIsFatal);
}

SqlBufferBase * sql_buffer_pool::addBuffer(Lng32 totalBufferSize,
					   SqlBufferBase::BufferType bufType,
					   bool failureIsFatal)
{
  return addBuffer(dynBufferList_, totalBufferSize, bufType,
                   failureIsFatal);
}

SqlBufferBase * sql_buffer_pool::addBuffer(Lng32 numTupps, Lng32 recordLength)
{
  return addBuffer((Lng32) SqlBufferNeededSize(numTupps,recordLength,
				       ((bufType_ == SqlBufferBase::DENSE_)
					? SqlBuffer::DENSE_ 
					: SqlBuffer::NORMAL_)));
}
  
SqlBufferBase *sql_buffer_pool::get_free_buffer(Lng32 freeSpace)
{
  return getBuffer(freeSpace,-1);
}

// RETURNS: 0, if tuple found. -1, if not found.
short sql_buffer_pool::get_free_tuple(tupp &tp, Lng32 tupDataSize,
                                      SqlBuffer **buf)
{
  if(buf)
    *buf = NULL;

  // there is a bug in SqlBufferDense::add_tuple_desc(), when
  // tup_data_size is 0, the tuple address of next allocated
  // tupp_descriptor will overlap to the tuple address of
  // previous allocated tupp_descriptor.
  // Until fixed, set tupDataSize to 8 if it is 0
  if (tupDataSize <= 0)
    tupDataSize = 8;

  Lng32 neededSpace = SqlBufferGetTuppSize(tupDataSize,bufType_);

  SqlBuffer *currBuffer;
  if (!(currBuffer =getCurrentBuffer(neededSpace)))
  {
    currBuffer = (SqlBuffer*)getBuffer(neededSpace);
  }
  
  if (currBuffer) // buffer found
  {
    tupp_descriptor *tuple_desc =
		currBuffer->allocate_tuple_desc(tupDataSize);

    // failed to set tp to a tuple_descriptor from the pool
    if (!tuple_desc)
      return -1;

    tuple_desc->setReferenceCount(0);

    tp = tuple_desc;
    
    if(buf)
      *buf = currBuffer;

    return 0;
      ///return tuple_desc;
  }
  
  // failed to set tp to a tuple_descriptor from the pool
  return -1;

}
//check if the current buffer in the pool has enough space
short  sql_buffer_pool::currentBufferHasEnoughSpace( Lng32 tupDataSize)
{
  SqlBuffer * currBuffer;
  if (!(currBuffer = (SqlBuffer *)(activeBufferList()->getCurr())))
  {
    return -1;
  }
  Lng32 neededSpace = SqlBufferGetTuppSize(tupDataSize,bufType_);

  if (neededSpace <= currBuffer->getFreeSpace())
    {
      return 1;
    }
    else
    {
      return 0;
    }
}

SqlBuffer * sql_buffer_pool::getCurrentBuffer()
{
  return ((SqlBuffer *)(activeBufferList()->getCurr()));
}
// returns a new tupp_descriptor in the pool
// that has a data length of tupDataSize.
// Sets the reference count to 1.
// RETURNS: tupp_descriptor, if tuple found. NULL, otherwise.
// If buf is non-Null, also sets buf to the buffer from which 
// the tupp_descriptor was allocated. This is used to later 
// resize the tuple if needed.
tupp_descriptor * sql_buffer_pool::get_free_tupp_descriptor(Lng32 tupDataSize,
                                                            SqlBuffer **buf)
{
  if(buf)
    *buf = NULL;

  Lng32 neededSpace = SqlBufferGetTuppSize(tupDataSize,bufType_);
  
  // get a buffer which is not FULL and can allocate tup_data_size 
  //SqlBuffer *currBuffer = (SqlBuffer*)getBuffer(neededSpace);
	// this change is not really related to CIF changes but it may imrove 
	// performance. I did a similar change to get_free_tuple function 
	// few years ago and it improved performance
  SqlBuffer *currBuffer;
  if (!(currBuffer =getCurrentBuffer(neededSpace)))
  {
    currBuffer = (SqlBuffer*)getBuffer(neededSpace);
  }
  
  if (currBuffer) // buffer found
    {
      // allocate a tuple_desc of the desired size and assign its
      // address to tp
      tupp_descriptor *tuple_desc =
	currBuffer->allocate_tuple_desc(tupDataSize);

      // failed to set tp to a tuple_descriptor from the pool
      if (!tuple_desc)
         return NULL;
      
      tuple_desc->setReferenceCount(1);

      if(buf)
        *buf = currBuffer;

      return tuple_desc;
    }
  else
    // failed to set tp to a tuple_descriptor from the pool
    return NULL;
}

void sql_buffer_pool::free_buffers()
{
  SqlBuffer *currBuffer;

  // position to the first entry
  activeBufferList()->position();
  while (currBuffer = ((SqlBuffer *)(activeBufferList()->getCurr())))
    {
      if (!currBuffer->freeBuffer()) 
	{
	  // couldn't free up this buffer.
	  // Well, at least compact it.
	  currBuffer->compactBuffer();
	}
      
      // move to the next entry
      activeBufferList()->advance();
    }
}

void sql_buffer_pool::compact_buffers()
{
  SqlBuffer *currBuffer;

  // position to the first entry
  activeBufferList()->position();
  while (currBuffer = ((SqlBuffer *)(activeBufferList()->getCurr())))
    {
      currBuffer->compactBuffer();

      // move to the next entry
      activeBufferList()->advance();
    }
}

// for debugging purposes
void sql_buffer_pool::printAllBufferInfo() {
  staticBufferList_->position();
  SqlBuffer * buf;
  while (buf = (SqlBuffer *)staticBufferList_->getNext())
    buf->printInfo();

  dynBufferList_->position();
  while (buf = (SqlBuffer *)dynBufferList_->getNext())
    buf->printInfo();
};

SqlBufferBase * sql_buffer_pool::findBuffer(Lng32 freeSpace,
					    Int32 mustBeEmpty)
{
  SqlBuffer *currBuffer;

  // walk the list until we find a buffer that we like.
  activeBufferList()->position();
  while (currBuffer = ((SqlBuffer *)(activeBufferList()->getCurr())))
    {
      // We like a buffer if it isn't in use by an I/O operation and
      // if it has enough space. Some callers also require that it must
      // be empty.
      if ((currBuffer->bufferStatus_ != SqlBuffer::IN_USE) &&
	  (!mustBeEmpty || (currBuffer->bufferStatus_ == SqlBuffer::EMPTY)) &&
	  (freeSpace <= currBuffer->getFreeSpace()))
	{
	  return currBuffer;
	}
      else
	// move to the next entry
	activeBufferList()->advance();
    }
  // out of luck
  return NULL;
}

SqlBuffer * sql_buffer_pool::getCurrentBuffer(Lng32 freeSpace,Int32 mustBeEmpty)
{
  SqlBuffer * currBuffer;
  if ((currBuffer = (SqlBuffer *)(activeBufferList()->getCurr())) &&
     (currBuffer->bufferStatus_ != SqlBuffer::IN_USE) &&
     (!mustBeEmpty || (currBuffer->bufferStatus_ == SqlBuffer::EMPTY)) &&
     (freeSpace <= currBuffer->getFreeSpace())) 
  {
    return currBuffer;
  }
  else
    return NULL;
}


SqlBufferBase * sql_buffer_pool::getBuffer(Lng32 freeSpace,
					Int32 mustBeEmpty)
{
  SqlBufferBase * result = findBuffer(freeSpace,mustBeEmpty);
  if (result)
    return result;
  
  // buffer not found. Free up buffers which do not have anyone
  // pointing to them and search again.
  free_buffers();

  result = findBuffer(freeSpace,mustBeEmpty);

  if (! result)
    {
      if (staticMode())
	{
	  // allocate a buffer for some static data structures,
	  // but try to use a denseer buffer size because it is
	  // likely that there won't be a lot of such static structures
	  // (guess that there are 10)
	  Lng32 neededStaticBufferSize;

	  if (freeSpace > 0)
	    neededStaticBufferSize = 
	      MINOF((Lng32) SqlBufferNeededSize(
		   10,
		   freeSpace,
		   bufType_),
		    defaultBufferSize_);
	  else
	    neededStaticBufferSize = defaultBufferSize_;

	  result = addBuffer(staticBufferList_,
			     neededStaticBufferSize,
			     bufType_);
	}
      else if (currNumberOfBuffers_ < maxNumberOfBuffers_)
      {
        // The requested size may be wrong if the buffer size isn't computed
        // correctly in codegen.  
        Lng32 bufferHeaderSize = (Lng32) SqlBufferHeaderSize(bufType_);

        // The space needed supplied by the caller (freeSpace) does not
        // account for the buffer header (buffer object) size. Add this
        // space to the needed space
        Lng32 buffSize = MAXOF(freeSpace+bufferHeaderSize,
                              defaultBufferSize_);

        // just add another buffer to the dynamic buffer list
        result = addBuffer(dynBufferList_, buffSize, bufType_);

        // Possible we can't even get the first buffer due to memory pressure.
        UdrExeAssert((result != NULL) && (get_number_of_buffers() != 0),
                     "Buffer size not computed correctly at codegen");
      }
    }

  return result;
}

void sql_buffer_pool::getUsedMemorySize(UInt32 &staticMemSize,
					UInt32 &dynMemSize)
{
  staticMemSize = sizeof(sql_buffer_pool);
  staticMemSize += 2 * sizeof(Queue);

  char * currBuffer;
  staticBufferList_->position();
  while (currBuffer = (char *)(staticBufferList_->getCurr()))
    {
      staticMemSize += ((SqlBuffer *)currBuffer)->get_used_size();

      // move to the next entry
      staticBufferList_->advance();
    }
     
  dynBufferList_->position();
  dynMemSize = 0;
  while (currBuffer = (char *)(dynBufferList_->getCurr()))
    {
      dynMemSize += ((SqlBuffer *)currBuffer)->get_used_size();

      // move to the next entry
      dynBufferList_->advance();
    }
}


void sql_buffer_pool::resizeLastTuple(UInt32 tup_data_size, char *dataPointer)
{
  SqlBuffer *currBuffer = (SqlBuffer *)(activeBufferList()->getCurr());

  currBuffer->resize_tupp_desc(tup_data_size, dataPointer);
  
}

SqlBufferHeader::moveStatus
sql_buffer_pool::moveIn(atp_struct *atp1,
                        atp_struct *atp2, 
                        UInt16 tuppIndex,
                        Lng32 tupDataSize, 
                        ex_expr_base *moveExpr,
                        NABoolean addBufferIfNeeded,
                        Lng32 bufferSize)
{
  if (defragTd_ &&
      !currentBufferHasEnoughSpace(tupDataSize))
  {
    // get row length after evaluating the expression and try to
    // to get new tuple with the actual row size. The actual row
    // size may be smaller that the max row size and may fit in the remaining
    // buffer space

    UInt32 defMaxRowLen = tupDataSize;
    UInt32 defRowLen = defMaxRowLen;
    defragTd_->setReferenceCount(1);
    atp2->getTupp(tuppIndex)= defragTd_;

    ex_expr::exp_return_type retCode;
    retCode = moveExpr->eval(atp1, atp2, 0, -1, &defRowLen);
    if (retCode == ex_expr::EXPR_ERROR)
    {
      return SqlBufferHeader::MOVE_ERROR;
    }
    if (!get_free_tuple(atp2->getTupp(tuppIndex), defRowLen))
    {
#if (defined(_DEBUG) )
      char txt[] = "hashj";
      SqlBuffer *buf = getCurrentBuffer();
      sql_buffer_pool::logDefragInfo(txt,
                              SqlBufferGetTuppSize(tupDataSize, buf->bufType()),
                              SqlBufferGetTuppSize(defRowLen, buf->bufType()),
                              buf->getFreeSpace(),
                              buf,
                              buf->getTotalTuppDescs());
#endif

       char * defragDataPointer = defragTd_->getTupleAddress();
       char * dataPointer = atp2->getTupp(tuppIndex).getDataPointer();
       str_cpy_all(dataPointer,
                   defragDataPointer,
                   defRowLen);
       return SqlBufferHeader::MOVE_SUCCESS;

    }
  }

  if (get_free_tuple(atp2->getTupp(tuppIndex), tupDataSize)) {
    if(addBufferIfNeeded) {
      ULng32 neededSize = SqlBufferNeededSize(1, bufferSize, bufType_);
      addBuffer(neededSize);
      if (get_free_tuple(atp2->getTupp(tuppIndex), tupDataSize)) {
        ex_assert(0, "sql_buffer_pool::moveIn() No more space for tuples");
      }
    } else {
      return SqlBufferHeader::BUFFER_FULL;
    }
  }

  UInt32 maxRowLen = tupDataSize;
  UInt32 rowLen = maxRowLen;
      
  ex_expr::exp_return_type retCode;
  retCode = moveExpr->eval(atp1, atp2, 0, -1, &rowLen);
  if (retCode == ex_expr::EXPR_ERROR)
    {
      return SqlBufferHeader::MOVE_ERROR;
    }

  // Resize the row if the actual size is different from the max size (leftRowLen)
  if(rowLen != maxRowLen) {
    resizeLastTuple(rowLen,
                    atp2->getTupp(tuppIndex).getDataPointer());
  }
  return SqlBufferHeader::MOVE_SUCCESS;
}


SqlBufferHeader::moveStatus
sql_buffer_pool::moveIn(atp_struct *atp,
                        UInt16 tuppIndex,
                        Lng32 tupDataSize, 
                        char *srcData,
                        NABoolean addBufferIfNeeded,
                        Lng32 bufferSize)
{
  if (get_free_tuple(atp->getTupp(tuppIndex), tupDataSize)) {
    if(addBufferIfNeeded) {
      addBuffer(bufferSize);
      if (get_free_tuple(atp->getTupp(tuppIndex), tupDataSize)) {
        ex_assert(0, "sql_buffer_pool::moveIn() No more space for tuples");
      }
    } else {
      return SqlBufferHeader::BUFFER_FULL;
    }
  }

  str_cpy_all(atp->getTupp(tuppIndex).getDataPointer(),
              srcData,
              tupDataSize);

  return SqlBufferHeader::MOVE_SUCCESS;
}



/////////////////////////////////////////////////////////////////////
// class SqlBufferOlt
//
// SqlBufferOlt is used to minimize the number of bytes replied 
// from dp2 to exe. A special version of this is SqlBufferOltSmall 
// which is used to send data from exe to dp2 and could also be used
// during reply if certain conditions are met. 
//
// An OLT buffer can send or reply atmost one data tuple.
// No down or up state is sent in the buffer. The OLT PA node
// remembers the down state info (parent index, etc) and returns
// that to its parent along with data returned by DP2.
//
// SqlBufferOlt has a header that has send/reply, buffer and content flags.
// For description of these flags, see the header file sql_buffer.h.
//
// Following the header, it can have data + warning diag or an error diag. 
// It can also have rows affected, row count or stats. Appropriate
// flags are set in the header. A tupp descriptor is allocated for each 
// of the above. 
//
// In the moveReply functions, the diagsArea can either be passed in 
// as a parameter or can be generated when evaluating an expression.
//
/////////////////////////////////////////////////////////////////////

void SqlBufferOlt::init(ULng32 in_size_in_bytes, NABoolean clear)
{
  SqlBufferBase::init(in_size_in_bytes, clear);

  oltBufFlags_ = 0;
  contents_ = NOTHING_YET_;
  filler_ = 0;
}

// reinitialize data members; note that this version of init()
// can only be used after a buffer has been initialized via 
// init(unsigned long).
void SqlBufferOlt::init()
{
  SqlBufferBase::init();

  oltBufFlags_ = 0;
  contents_ = NOTHING_YET_;
  filler_ = 0;
}

// converts all pointers in tuple descriptors to offset relative
// to the beginning of SqlBuffer. NOTE: SqlBuffer is not derived
// from the ExGod class but has similar pack and unpack methods.
void SqlBufferOlt::pack()
{
  if (containsOltSmall())
    return;

  Int32 numTupps = 0;
  switch (getContents())
    {
    case ERROR_:
    case DATA_:
    case NODATA_ROWCOUNT_:
    case NODATA_WARNING_:
    case NODATA_ROWAFFECTED_WARNING_:
      numTupps = 1;
      break;

    case DATA_WARNING_:
    case NODATA_ROWCOUNT_WARNING_:
      numTupps = 2;
      break;
    }

  if (isStats())
    numTupps++;

  tupp_descriptor * nextTdesc = NULL;
  tupp_descriptor * tdesc = getNextTuppDesc(NULL);
  Int32 i = 0;
  while (i++ < numTupps)
    {
      if (i < numTupps)
	nextTdesc = getNextTuppDesc(tdesc);
      tdesc->setTupleOffset((char *)tdesc->getTupleAddress() -
			    (char *)this);
      tdesc = nextTdesc;
    }

  SqlBufferBase::pack();
}

void SqlBufferOlt::unpack()
{
  Int32 numTupps = 0;
  switch (getContents())
    {
    case ERROR_:
    case DATA_:
    case NODATA_ROWCOUNT_:
    case NODATA_WARNING_:
    case NODATA_ROWAFFECTED_WARNING_:
      numTupps = 1;
      break;

    case DATA_WARNING_:
    case NODATA_ROWCOUNT_WARNING_:
      numTupps = 2;
      break;
    }

  if (isStats())
    numTupps++;

  tupp_descriptor * tdesc = NULL;
  for (Int32 i = 0; i < numTupps; i++)
    {
      tdesc = getNextTuppDesc(tdesc);
      tdesc->setTupleAddress((char *)this + tdesc->getTupleOffset());
    }

  SqlBufferBase::unpack();
}

SqlBufferBase::moveStatus 
  SqlBufferOlt::moveInSendData(ULng32 projRowLen,
			       ex_expr_base * expr, 
			       atp_struct * atp1,
			       atp_struct * workAtp, 
			       unsigned short tuppIndex)
{
  SqlBufferOltSmall * smallBuf = (SqlBufferOltSmall*)getSendDataPtr();
  smallBuf->setBufType(SqlBufferHeader::OLT_SMALL_);
  smallBuf->init();

  if (projRowLen > 0)
    {
      smallBuf->setSendData(TRUE);

      tupp_descriptor td;
      td.init(projRowLen, NULL, smallBuf->sendDataPtr());

      workAtp->getTupp(tuppIndex) = &td;
      ex_expr::exp_return_type rc = expr->eval(atp1, workAtp);
      workAtp->getTupp(tuppIndex).release();

      if (rc == ex_expr::EXPR_ERROR)
	{
	  return MOVE_ERROR;
	} // error
    }

  // since we are not allocating a tupp desc to indicate the input
  // row length, set this value in the sizeInBytes_ field.
  sizeInBytes_ = sizeof(SqlBufferOltSmall) + projRowLen;
  
  return MOVE_SUCCESS;
}

SqlBufferBase::moveStatus 
  SqlBufferOlt::moveInReplyData(NABoolean doMoveControl,
				NABoolean doMoveData,
				void * currQState,
				ULng32 projRowLen,
				ComDiagsArea * diagsArea,
				tupp_descriptor ** diagsDesc,
				ex_expr_base * expr, 
				atp_struct * atp1,
				atp_struct * workAtp, 
				atp_struct * destAtp,
				unsigned short tuppIndex,
				NABoolean doMoveStats, 
				ExStatisticsArea * statsArea,
				tupp_descriptor ** statsDesc)
{
  if ((((up_state *)currQState)->matchNo > 1) || (diagsArea) ||
      (getContents() == ERROR_) || (getContents() == DATA_WARNING_) ||
      ((statsArea) && (NOT statsArea->smallStatsObj())))
    {
      return moveInSendOrReplyData(FALSE, // reply
				   doMoveControl, doMoveData, currQState,
				   0, NULL,
				   projRowLen, NULL, 
				   diagsArea, diagsDesc,
				   expr, atp1, workAtp, destAtp,
				   tuppIndex, 
				   doMoveStats, statsArea, statsDesc);
    }

  if (doMoveData)
    setDataProcessed(TRUE);

  setContainsOltSmall(TRUE);

  SqlBufferOltSmall * smallBuf = (SqlBufferOltSmall*)getReplyDataPtr();
  if ((doMoveData) ||
      (doMoveControl && (NOT dataProcessed())))
    {
      smallBuf->setBufType(SqlBufferHeader::OLT_SMALL_);
      smallBuf->init();

      // since we are not allocating a tupp desc to indicate the input
      // row length, set this value in the sizeInBytes_ field.
      sizeInBytes_ = sizeof(SqlBufferOltSmall);
    }

  if (((up_state *)currQState)->status == ex_queue::Q_STATS)
    {
      smallBuf->setReplyStatsOnly(TRUE);
      UdrExeAssert(doMoveStats && statsArea && statsDesc && *statsDesc, 
                   "Incorrect parameters for Q_STATS");
    }
  else if (((up_state *)currQState)->matchNo > 0)
    smallBuf->setReplyRowAffected(TRUE);

  if (doMoveData)
    {
      smallBuf->setReplyData(TRUE);

      if (expr)
	{
	  atp_struct * atp = ((destAtp != workAtp) ? destAtp : workAtp);
	  tupp_descriptor td;
	  NABoolean tuppAllocated = FALSE;

	  if (atp->getTupp(tuppIndex).isAllocated())
	    {
	      atp->getTupp(tuppIndex).setDataPointer((char *)smallBuf+sizeInBytes_);
	    }
	  else
	    {
	      tuppAllocated = TRUE;
	      td.init(projRowLen, NULL, (char *)smallBuf+sizeInBytes_);
	      atp->getTupp(tuppIndex) = &td;
	    }

	  ex_expr::exp_return_type rc = ex_expr::EXPR_OK;
	  if (expr)
	    rc = expr->eval(atp1, workAtp);

	  if (tuppAllocated)
	    {
	      atp->getTupp(tuppIndex).release();
	    }

	  // an error or warning was returned by expr eval method.
	  // Cannot use small olt buf.
	  if (atp1->getDiagsArea())
	    {
	      // clean diags area and call moveInSendOrReplyData.
	      // It will reevaluate the expression.
	      atp1->getDiagsArea()->clear();
	      
	      setContainsOltSmall(FALSE);

	      return moveInSendOrReplyData(
		   FALSE, // reply
		   doMoveControl, doMoveData, currQState,
		   0, NULL,
		   projRowLen, NULL, 
		   atp1->getDiagsArea(), diagsDesc,
		   expr, atp1, workAtp, destAtp,
		   tuppIndex, 
		   doMoveStats, statsArea, statsDesc);
	    } // error or warning
	} // expr passed in

      sizeInBytes_ += projRowLen;
    }

  if ((doMoveStats) && (statsArea) && (statsDesc) && (*statsDesc))
    {
      // allocate space for stats area.
      short statsAreaLen = (short)statsArea->packedLength();

      sizeInBytes_ = ROUND2(sizeInBytes_);
      *(short*)((char *)smallBuf + sizeInBytes_)
	= statsAreaLen;

      sizeInBytes_ += sizeof(short);

      char * statsAreaLoc = 
	(statsArea->smallStatsObj() ? ((char *)smallBuf+sizeInBytes_)
	 : (char*)( ROUND8 (Long((char*)smallBuf+sizeInBytes_) ) ));

      (*statsDesc)->init(statsAreaLen, 0, statsAreaLoc);
      
      sizeInBytes_ = (statsAreaLoc - (char *)smallBuf) + statsAreaLen;
      smallBuf->setReplyStats(TRUE);
    }

  return MOVE_SUCCESS;
}

SqlBufferBase::moveStatus 
  SqlBufferOlt::moveInSendOrReplyData(NABoolean isSend,
				      NABoolean doMoveControl,
				      NABoolean doMoveData,
				      void * currQState,
				      ULng32 controlInfoLen,
				      ControlInfo ** controlInfo,
				      ULng32 projRowLen,
				      tupp_descriptor ** outTdesc,
				      ComDiagsArea * diagsArea,
				      tupp_descriptor ** diagsDesc,
				      ex_expr_base * expr, 
				      atp_struct * atp1,
				      atp_struct * workAtp, 
				      atp_struct * destAtp,
				      unsigned short tuppIndex,
				      NABoolean doMoveStats, 
				      ExStatisticsArea * statsArea,
				      tupp_descriptor ** statsDesc,
				      NABoolean useExternalDA,
				      NABoolean callerHasExternalDA,
				      tupp_descriptor * defragTd
#if (defined(_DEBUG) )
				      ,ex_tcb * tcb
#endif
				       ,NABoolean noMoveWarnings
                                      )
{
  // this should be used for reply only.
  if (isSend)
    return MOVE_ERROR;

  if ((! diagsArea) && (((up_state *)currQState)->matchNo <= 1) &&
      (getContents() != ERROR_) && (getContents() != DATA_WARNING_) &&
      ((! statsArea) || (statsArea->smallStatsObj())))

    {
      return moveInReplyData(doMoveControl, doMoveData, currQState,
			     projRowLen, diagsArea, diagsDesc,
			     expr, atp1, workAtp, destAtp, tuppIndex,
			     doMoveStats, statsArea, statsDesc);
    }

  setContainsOltSmall(FALSE);

  SqlBuffer::moveStatus retcode = MOVE_SUCCESS;
  tupp_descriptor * tdesc = NULL;

  // Genesis 10-040126-2683 and 10-040126-0354.
  // On entry into this function, sizeInBytes_ represents the max size of the
  // buffer. This function will later change it to the actual number of bytes
  // used. Note the max size so that we can ensure the number of bytes used does
  // not exceed this limit.
  ULng32 maxBufSize = sizeInBytes_;

  if ((! doMoveData) && (getContents() != NOTHING_YET_))
    {
      // either one tupp(data or error) or two tupps (data + warning)
      // have been moved in. Skip them.
      tdesc = getNextTuppDesc(tdesc); // skip data or error
      if (getContents() == DATA_WARNING_)
	tdesc = getNextTuppDesc(tdesc); // skip the warning tupp
    }

  if (doMoveData) // (doMoveData) && (projLen > 0))
    {
      setContents(DATA_);

      tdesc = getNextTuppDesc(tdesc, projRowLen);
     
      if (outTdesc != NULL)
	*outTdesc = tdesc;
      
      if (destAtp)
	destAtp->getTupp(tuppIndex) = tdesc;

      if ((expr != NULL) &&
	  ((expr->eval(atp1, workAtp)) == ex_expr::EXPR_ERROR))
	{
	  if ((destAtp) && (workAtp == destAtp))
	    destAtp->release();
	  	  
	  retcode = MOVE_ERROR;
	} // expr present and error
      else
	setDataProcessed(TRUE);

      // the expr eval could have produced a warning or error diag.
      // copy it to the diagsArea parameter. A tupp for diag will
      // be allocated below based on the diagsArea. 
      if (diagsArea == NULL)
        diagsArea = atp1->getDiagsArea();
    }
  else if ((doMoveControl) && (getContents() == NOTHING_YET_))
    //  else if ((doMoveControl) && (NOT dataProcessed()))
    {
      if (((up_state *)currQState)->status == ex_queue::Q_STATS)
        {
          setContents(STATSONLY_);
      UdrExeAssert(doMoveStats && statsArea && statsDesc && *statsDesc, 
                       "Incorrect parameters for Q_STATS");
        }
      else if (((up_state *)currQState)->matchNo == 0)
	setContents(NODATA_);
      else if (((up_state *)currQState)->matchNo == 1)
	setContents(NODATA_ROWAFFECTED_);
      else
	{
	  setContents(NODATA_ROWCOUNT_);
	  tdesc = getNextTuppDesc(tdesc, sizeof(Lng32));
	  *(Lng32*)tdesc->getTupleAddress() = ((up_state *)currQState)->matchNo;
	}
    }

  if (diagsArea != NULL)
    {
      if (diagsArea->mainSQLCODE() < 0)
	{
	  setContents(ERROR_);

	  // allocate space for error diags area.
	  tdesc = getNextTuppDesc(NULL, diagsArea->packedLength());
	}
      else
	{
	  // Genesis 10-040126-2683 and 10-040126-0354.
	  // When multiple warnings are generated, the total size of the packed
	  // buffer sometimes exceeds the allocated size. When this happens, the
	  // receiving mxci process gets only a partial buffer. And when this
	  // partial buffer is unpacked, it causes an assertion failure. Since
	  // the buffers cant be resized, we have to ensure the diags area
	  // always fits into the space available. So, if there isnt enough
	  // space, we will first try reducing the number of warnings to just
	  // one. If there isnt space for even a single warning, the diags area
	  // wont be packed at all.
	  if ((UInt32) sizeInBytes_ == maxBufSize)
	  {
	    // No data rows are being returned. This diags area is the first
	    // thing to be packed into the buffer.
	    sizeInBytes_ = 0;
	  }

	  if (diagsArea->packedLength() + sizeInBytes_ > maxBufSize)
	  {
	    // We cannot fit all the warnings into the reply buffer.
	    // Delete all but the first warning.
	    while (diagsArea->getNumber(DgSqlCode::WARNING_) > 1)
	      diagsArea->deleteWarning(1);

	    if (diagsArea->packedLength() + sizeInBytes_ > maxBufSize)
	    {
	      // No space for any warnings.
	      // Should never come here because the buffer should always be big
	      // enough to contain at least one warning.
	      diagsArea->deleteWarning(0);
	    }
	  }

	// move diags area plus warning(s)
	tdesc = getNextTuppDesc(tdesc, diagsArea->packedLength());

	switch (getContents())
	  {
	  case DATA_: 
	    setContents(DATA_WARNING_);
	    break;
	  case NODATA_: 
	    setContents(NODATA_WARNING_); 
	    break;
	  case NODATA_ROWAFFECTED_: 
	    setContents(NODATA_ROWAFFECTED_WARNING_); 
	    break;
	  case NODATA_ROWCOUNT_: 
	    setContents(NODATA_ROWCOUNT_WARNING_); 
	    break;

	  }
	}

      if (diagsDesc)
        *diagsDesc = tdesc;
    }

  if ((doMoveStats) && (statsArea != NULL))
    {
      // allocate space for stats area.
      tdesc = getNextTuppDesc(tdesc, statsArea->packedLength());

      if (statsDesc)
	*statsDesc = tdesc;

      setIsStats(TRUE);
    }

  return retcode;
}

NABoolean SqlBufferOlt::moveOutSendOrReplyData(NABoolean isSend,
					       void * currQState,
					       tupp &outTupp,
					       ControlInfo ** controlInfo,
					       ComDiagsArea ** diagsArea,
					       ExStatisticsArea ** statsArea,
					       Int64 * numStatsBytes,
					       NABoolean noStateChange,
					       NABoolean unpackDA,
					       CollHeap * heap)
{
  tupp_descriptor * tDesc = NULL;

  switch (getContents())
    {
    case ERROR_:
      {
	tDesc = getNextTuppDesc(tDesc);

	*diagsArea = (ComDiagsArea *)(tDesc->getTupleAddress());
	
	((up_state *)currQState)->status = ex_queue::Q_SQLERROR;
      }
    break;

    case DATA_:
      {
	tDesc = getNextTuppDesc(tDesc);

	outTupp = tDesc;
	((up_state *)currQState)->status = ex_queue::Q_OK_MMORE;
	((up_state *)currQState)->matchNo = 1;
      }
    break;

    case DATA_WARNING_:
      {
	tDesc = getNextTuppDesc(tDesc);

	outTupp = tDesc;
	((up_state *)currQState)->status = ex_queue::Q_OK_MMORE;
	((up_state *)currQState)->matchNo = 1;

	tDesc = getNextTuppDesc(tDesc);
	*diagsArea = (ComDiagsArea *)(tDesc->getTupleAddress());
      }
    break;

    case NODATA_:
      {
	((up_state *)currQState)->status = ex_queue::Q_NO_DATA;
	((up_state *)currQState)->matchNo = 0;
      }
    break;

    case NODATA_ROWAFFECTED_:
      {
	((up_state *)currQState)->status = ex_queue::Q_NO_DATA;
	((up_state *)currQState)->matchNo = 1;
      }
    break;

    case NODATA_ROWCOUNT_:
      {
	((up_state *)currQState)->status = ex_queue::Q_NO_DATA;

	tDesc = getNextTuppDesc(tDesc);
	((up_state *)currQState)->matchNo = *(Lng32*)tDesc->getTupleAddress();
      }
    break;

    case NODATA_WARNING_:
      {
	((up_state *)currQState)->status = ex_queue::Q_NO_DATA;
	((up_state *)currQState)->matchNo = 0;

	tDesc = getNextTuppDesc(tDesc);
	*diagsArea = (ComDiagsArea *)(tDesc->getTupleAddress());
      }
    break;

    case NODATA_ROWAFFECTED_WARNING_:
      {
	((up_state *)currQState)->status = ex_queue::Q_NO_DATA;
	((up_state *)currQState)->matchNo = 1;

	tDesc = getNextTuppDesc(tDesc);
	*diagsArea = (ComDiagsArea *)(tDesc->getTupleAddress());
      }
    break;

    case NODATA_ROWCOUNT_WARNING_:
      {
	((up_state *)currQState)->status = ex_queue::Q_NO_DATA;

	tDesc = getNextTuppDesc(tDesc);
	((up_state *)currQState)->matchNo = *(Lng32*)tDesc->getTupleAddress();

	tDesc = getNextTuppDesc(tDesc);
	*diagsArea = (ComDiagsArea *)(tDesc->getTupleAddress());
      }
    break;

    case STATSONLY_:
      {
        ((up_state *)currQState)->status = ex_queue::Q_STATS;
      }
    break;
    }
  
  if ((isStats()) && (statsArea))
    {
      tDesc = getNextTuppDesc(tDesc);
      *statsArea = (ExStatisticsArea *)(tDesc->getTupleAddress());
      *numStatsBytes = sizeof(*tDesc) + tDesc->getAllocatedSize();
    }

  return FALSE;
}

// add a new tuple descriptor to the end of the buffer
tupp_descriptor *SqlBufferOlt::add_tuple_desc(Lng32 tup_data_size)
{
  return NULL;
}


// remove the tuple desc with the highest number from the buffer
void SqlBufferOlt::remove_tuple_desc()
{
}

////////////////////////////////////////////////////////////////////////////
// class SqlBufferOltSmall
////////////////////////////////////////////////////////////////////////////
NABoolean SqlBufferOltSmall::moveOutSendData(tupp &outTupp,
					     ULng32 returnedRowLen)
{
  if (sendData())
    {
      // this input value didn't come with a tupp descriptor.
      // Allocate one now in this buffer.
      tupp_descriptor * td = 
	(tupp_descriptor *)((char*)this+
				    sizeof(SqlBufferOltSmall)+
				    ROUND8(returnedRowLen));
      td->init(returnedRowLen, NULL, (char *)this+sizeof(SqlBufferOltSmall));
      outTupp = td;
    }

  return FALSE;
}

NABoolean SqlBufferOltSmall::moveOutReplyData(void * currQState,
					      tupp &outTupp,
					      ULng32 returnedRowLen,
					      ComDiagsArea ** diagsArea,
					      ExStatisticsArea ** statsArea,
					      Int64 * numStatsBytes)
{
  Long replyDataLoc = 0;
  Long statsAreaLoc = 0;
  Long dataLoc = 0;
  short statsAreaLen = 0;
  if ((replyData()) || (replyStats()))
    {
      dataLoc = (Long)this + sizeof(SqlBufferOltSmall);
      if (replyData())
	{
	  replyDataLoc = dataLoc;
	  if (replyStats())
	    dataLoc = dataLoc + ROUND2(returnedRowLen);
	  else
	    dataLoc = dataLoc + ROUND4(returnedRowLen);
	}

      if (replyStats())
	{
	  statsAreaLen = *(short*)dataLoc;
	  statsAreaLoc = dataLoc + sizeof(short);
	  //	  statsAreaLoc = ROUND8(statsAreaLoc);
	  dataLoc = statsAreaLoc + ROUND8(statsAreaLen);
	}
    }

  if (replyData())
    { 
      // this reply value didn't come with a tupp descriptor.
      // Allocate one now in this buffer.
      tupp_descriptor * td = (tupp_descriptor *)((char*)dataLoc);
      td->init(returnedRowLen, NULL, (char *)replyDataLoc);
      outTupp = td;
      
      ((up_state *)currQState)->status = ex_queue::Q_OK_MMORE;
    }
  else if (replyStatsOnly())
    {
      ((up_state *)currQState)->status = ex_queue::Q_STATS;
    }
  else
    {
      ((up_state *)currQState)->status = ex_queue::Q_NO_DATA;
    }

  if (replyRowAffected())
    ((up_state *)currQState)->matchNo = 1;
  else
    ((up_state *)currQState)->matchNo = 0;

  if (replyStats())
    {
      *statsArea = (ExStatisticsArea *)statsAreaLoc;
      *numStatsBytes = sizeof(short) + statsAreaLen;
    }
  
  return FALSE;
}

#if (defined(_DEBUG) )
void sql_buffer_pool::logDefragInfo(char * txt,
                                    Lng32 neededSpace,
                                    Lng32 actNeededSpace,
                                    Lng32 freeBuffSpace,
                                    void *p,
                                    Lng32 NumRowsInBuff,
                                    ex_tcb * tcb)
{


   char * envCifLoggingLocation= getenv("CIF_DEFRAG_LOG_LOC");
   if (envCifLoggingLocation)
   {
     char file2 [255];
     snprintf(file2,255,"%s%s",envCifLoggingLocation,"/cif_defrag_logging.log");
     FILE * p2 = NULL;
     p2 =fopen(file2, "a");
     if (p2== NULL)
     {
       printf("Error in opening a file..");  // file2
     }
     //fprintf(p2,"%s\n","defragmentation");
     time_t rawtime;
     struct tm * timeinfo;
     time ( &rawtime );

     timeinfo = localtime ( &rawtime );
     fprintf(p2,"%s      %s explainId: %d buffer: %p Neededspace: %d AcualSpace: %d freeSapce: %d NumRowsInBuff: %d\n",
         asctime (timeinfo),
         tcb ? tcb->getTdb()->getNodeName() : txt,
         tcb ? tcb->getTdb()->getExplainNodeId() : -1,
         p,neededSpace,actNeededSpace,freeBuffSpace,NumRowsInBuff);

     fclose(p2);
   }
}

#endif
#endif //UDRSERV_BUILD


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
#ifndef SQL_BUFFER_H
#define SQL_BUFFER_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         sql_buffer.h
 * Description:  SQL buffers are containers for tupps. SQL buffer pools
 *               manage the allocation and deallocation of sql buffers.
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

//
// NOTE: Some of the code in this file is excluded from the UDR server
// build using the UDRSERV_BUILD macro. The excluded code is:
//  - All of the sql_buffer_pool, SqlBufferOlt, and SqlBufferOltSmallcode
//  - Code in the SqlBuffer class that deals with ATPs, expression
//    evaluation, and statistics
//

#include "ExpSqlTupp.h"
#include "ex_io_control.h"
#ifndef UDRSERV_BUILD
#include "ex_god.h"
#endif // UDRSERV_BUILD

// Defining SQL_BUFFER_SIGNATURE gives each sql_buffer a unique
// sequence number and a process ID.  The seq number is unique within
// the process.  It's useful for tracing sql_buffers to/from
// executor<-->exe-in-dp2 via the ArkFs.  But, it changes the size of
// the sql_buffer, and thus really should be used only if guarded by
// an EIDVersion change, because EID must know about this signature
// too.  Coordination w/ EIDVersion change is not done unfortunately.
// So if you use this, be sure to remember to rebuild dp2.  

//#define SQL_BUFFER_SIGNATURE

#ifdef SQL_BUFFER_SIGNATURE
#include "Int64.h"
#endif

// Integrity checking -- do this for debug builds only.  Can also be enabled
// for testing release code, but should not be enabled for customers or 
// Performance testing.
#if defined(_DEBUG)
#define DO_INTEGRITY_CHECK
#endif

#include "ComDefs.h"              // pick up ROUND8 macro
#define NO_INVALID_TUPLES USHRT_MAX  // 0xffff, maximum unsigned short value, defined in limits.h
/////////////////////////////////////////////////////////////////////
// contents of this file
////////////////////////////////////////////////////////////////////

class sql_buffer_pool;
class ExStatisticsArea;
class atp_struct;
class ex_expr;

/////////////////////////////////////////////////////////////////////
// forward references
////////////////////////////////////////////////////////////////////
class Queue;
class SqlBufferOlt;
class SqlBufferDense;
class SqlBufferNormal;
class SqlBuffer;

typedef ex_expr ex_expr_base;

// this class used in SqlDenseBuffer
class TupleDescInfo : public tupp_descriptor
{
public:

  tupp_descriptor * tupleDesc() { return (tupp_descriptor*)this; };


  ULng32 getNextTDIOffset() { return nextTDIOffset(); };

  ULng32 getPrevTDIOffset() { return prevTDIOffset(); };

  void setNextTDIOffset(ULng32 tdiOffset)
  {
    nextTDIOffset() = tdiOffset;
  }

  void setPrevTDIOffset(ULng32 tdiOffset)
  {
    prevTDIOffset() = tdiOffset;
  }


  void pack(Long base);

  void unpack(Long base);
};

/////////////////////////////////////////////////////////////////////////////
//
//                  SqlBufferHeader
//                       /     \
//                      /       \
//                     /         \
//               SqlBufferBase    SqlBufferOltSmall
//                 /          \
//                /            \
//             SqlBuffer       SqlBufferOlt
//             /        \
//            /          \
//    SqlBufferNormal   SqlBufferDense
//
//
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// class SqlBufferHeader
//
// NOTE**** DO NOT add any member fields or virtual methods to this class.
//
///////////////////////////////////////////////////////////////////////////
class SqlBufferHeader
{
public:
  enum BufferType
  {
    NORMAL_    = 0,
    DENSE_     = 1,
    OLT_       = 2,
    OLT_SMALL_ = 3
  };

  // Buffer status
  enum buffer_status_type
  {
    FULL = 1,      // no more space in buffer
    EMPTY = 2,     // no records in buffer
    PARTIAL = 3,   // some records in buffer
    IN_USE = 4     // buffer is being transferred from/to FS2 or DP2
  };

  enum moveStatus
  {
    BUFFER_FULL,
    MOVE_SUCCESS,
    MOVE_ERROR
  };

  SqlBufferHeader(BufferType bufType)
       : bufType_(bufType)
  {
    init();
  };

  void init()
  {
    sendFlags_ = 0;
  };


  NABoolean denseBuffer() { return ((bufType_ == DENSE_) != 0); };


  void setDenseBuffer(NABoolean v)
  { ( v ? bufType_ = DENSE_ : bufType_ = NORMAL_ ); }


  NABoolean oltBuffer() { return ((bufType_ == OLT_) != 0); };


  void setOltBuffer(NABoolean v)
  { ( v ? bufType_ = OLT_ : bufType_ = NORMAL_ ); }


  BufferType bufType() { return (BufferType)bufType_; };


  void setBufType(BufferType bt) { bufType_ = bt; };


  NABoolean statsEnabled() { return ((sendFlags_ & STATS_ENABLED) != 0); };


  void setStatsEnabled(NABoolean v)
  { (v ? sendFlags_ |= STATS_ENABLED : sendFlags_ &= ~STATS_ENABLED); };

protected:
  unsigned char& sendFlags()  { return sendFlags_;  }
  unsigned char& replyFlags() { return replyFlags_; }

private:
  enum SendFlags
  {
    // The kind of stats to be collected is set at compile time based
    // on a CQD. This could be overridden at runtime by the next flag.
    // If set to 1, stats collection is enabled at runtime. If 0, it is not.
    // Set whenever an input sql buffer is sent from exe->esp or fs->dp2.
    // Currently being used to enable or disable measure stats collection
    // at runtime based on whether measure is enabled.
    STATS_ENABLED = 0x80
  };
  unsigned char           bufType_;
  union
  {
    unsigned char           sendFlags_;
    unsigned char           replyFlags_;
  };
};

class SqlBufferBase : public SqlBufferHeader
{
public:
  friend class sql_buffer_pool;
  friend class SqlTable;

   SqlBufferBase(BufferType bufType);

   // fixup vtbl ptr and inits.
  void driveInit(ULng32 in_size_in_bytes, 
			    NABoolean clear,
			    BufferType bt);

  // fixup vtbl ptr and inits.
  void driveInit(); 

  // fixup vtbl ptr and unpacks.
  void driveUnpack();

  // packs.
  void drivePack();

  // fixup vtbl ptr and calls the verify() method
  NABoolean driveVerify(ULng32 maxBytes);

  void fixupVTblPtr();

  // -------------------------------------------------------------------
  // returns sizeof the clase. Used at initialization time to
  // compute free space, etc.
  // All subclasses MUST redefine this method to return the correct
  // object sizes.
  // -------------------------------------------------------------------
  virtual Lng32 getClassSize(){return sizeof(SqlBufferBase);};

  // initialize data members after allocation, given the total length
  // in bytes of the allocated sql_buffer
  virtual void init(ULng32 in_size_in_bytes,
			       NABoolean clear = FALSE);

  // reinitialize data members; note that this version of init()
  // can only be used after a buffer has been initialized via 
  // init(unsigned long).
  virtual void init();

  // converts all pointers in tuple descriptors to offset relative
  // to the beginning of sql_buffer. NOTE: sql_buffer is not derived
  // from the ExGod class but has similar pack and unpack methods.
  virtual void pack();

  virtual void unpack();

  // A method to call before unpacking to verify the internal
  // consistency of a packed object. The main concern is to make sure
  // that after we convert offsets to pointers, we are not referencing
  // memory outside the bounds of the incoming message
  // buffer. Currently verify() is only called by the ExUdrTcb class
  // when it receives a data reply from the non-trusted UDR server.
  virtual NABoolean verify(ULng32 maxBytes) const;

  virtual ULng32 get_used_size()
    { return 0; }

  // number of bytes to be sent across processes. From FS to EID,
  // or from Send Top to Send Bottom.
  // For SqlBufferDense,
  // this method is redefined to return the actual number of bytes to
  // be sent.
  virtual ULng32 getSendBufferSize()
    { return 0; }

  // number of bytes to be replied across processes. From EID to FS,
  // of from Send Bottom to Send Top.
  // For SqlBufferDense,
  // this method is redefined to return the actual number of bytes to
  // be sent.
  virtual ULng32 getReplyBufferSize()
    { return getSendBufferSize(); }


  NABoolean packed() { return ((baseFlags_ & PACKED) != 0); };
  

  void setPacked(NABoolean v) { (v ? baseFlags_ |= PACKED : baseFlags_ &= ~PACKED); };


  void * operator new(size_t size, char * s);


  // set the buffer status
  inline void bufferInUse()            {bufferStatus_ = IN_USE;}
  inline void bufferFull()               {bufferStatus_ = FULL;}
  inline void setBufferStatus(buffer_status_type buf_stat)
                                                {bufferStatus_ = buf_stat;}

  // get the buffer status 
  inline Int32 isFull() const    {return (bufferStatus_ == FULL);}
  inline Int32 isEmpty() const  {return (bufferStatus_ == EMPTY);}


  
  virtual moveStatus 
  moveInSendOrReplyData(NABoolean isSend,   // TRUE = send
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
			ex_expr_base * expr = NULL, 
			atp_struct * atp1 = NULL,
			atp_struct * workAtp = NULL, 
			atp_struct * destAtp = NULL, 
			unsigned short tuppIndex = 0,
			NABoolean doMoveStats = FALSE, // if TRUE & statsArea
			                              // is passed in,  
			                              // move in stats
			ExStatisticsArea * statsArea = NULL,
			tupp_descriptor ** statsDesc = NULL,
			NABoolean useExternalDA = FALSE,
			NABoolean callerHasExternalDA = FALSE,
			tupp_descriptor * defragTd = NULL
#if (defined(_DEBUG))
			,ex_tcb * tcb = NULL
#endif
			,NABoolean noMoveWarnings = FALSE
			);
  
  
  virtual NABoolean moveOutSendOrReplyData(NABoolean isSend,
					   void * currQState,
					   tupp &outTupp,
					   ControlInfo ** controlInfo,
					   ComDiagsArea ** diagsArea,
					   ExStatisticsArea ** statsArea = NULL,
					   Int64 * numStatsBytes = NULL,
					   NABoolean noStateChange = FALSE,
					   NABoolean unpackDA = FALSE,
					   CollHeap * heap = NULL);
  
  virtual SqlBuffer * castToSqlBuffer() { return NULL; }

  
  virtual SqlBufferNormal * castToSqlBufferNormal() { return NULL; }

  
  virtual SqlBufferDense * castToSqlBufferDense() { return NULL; }

  
  virtual SqlBufferOlt * castToSqlBufferOlt() { return NULL; }

  // total size in bytes
  inline ULng32 get_buffer_size() const
                                                   { return sizeInBytes_; }

  void setBufferSize(ULng32 s)
  {
    sizeInBytes_ = s;
  }

  // positions to the first tupp descriptor in the buffer.
  virtual void position();

  // positions to the "tupp_desc_num"th tupp descriptor.
  virtual void position(Lng32 tupp_desc_num);
  
  // returns the 'next' tupp descriptor. Increments the
  // position. Returns null, if none found.
  virtual tupp_descriptor * getNext();

  // returns the 'current' tupp descriptor. Does not increment
  // position. Returns null, if none found.
  virtual tupp_descriptor * getCurr();

  // advances the current tupp descriptor
  virtual void advance();

  // add a new tuple descriptor to the end of the buffer
  virtual tupp_descriptor *add_tuple_desc(Lng32 tup_data_size);

  // remove the tuple desc with the highest number from the buffer
  virtual void remove_tuple_desc();

  // is space for a tuple with the given length available?
  virtual short space_available(Lng32 tup_data_size)
  {
    return 0;
  }

  // Next method checks integrity of a reply sql buffer when it 
  // leaves exe-in-dp2 and arrives at a partition access.
  // TBD -- 1) enhance this to be usable for reply buffers 
  // from producer ESPs (which send diags and stats externally)
  // and 2) enhance to be usabe for buffers sent to exe-in-dp2 and
  // producer ESPs.
  // This is a no-op for the OLT subclasses of sql buffer, and in 
  // release code.
#ifdef DO_INTEGRITY_CHECK
  virtual void integrityCheck( NABoolean ignore_omm_check=FALSE) { return; };
#else
  // Cannot afford to check reply_buffer for release code.
  inline void integrityCheck( NABoolean ignore_omm_check=FALSE) { return; };
#endif

protected:
 enum Flags 
  {
    IGNORE_CONTROL_FLAG = 0x0001, 
    PACKED = 0x0002,
    CURR_HAS_EDA = 0x0004,  // current entry associated w/ external diags area
    RETURN_ROWCOUNT = 0x0008,
    ROWCOUNT_RETURNED = 0x0010,
    SAME_EXECUTION = 0x0020,
    VAR_LEN_ROWS = 0x0040,  // currently only set for normal buffers
    FIRST_EXECUTION = 0x0080
  };
  
  unsigned char  baseFlags_;
  char           bufferStatus_;
  Lng32		 sizeInBytes_;        // total size of the buffer

};

/*
unsigned long SqlBufferNeededSize(long numTuples = 0, 
				  long recordLength = 0,
				  SqlBufferHeader::BufferType bufType = SqlBufferHeader::NORMAL_);

 
*/
////////////////////////////////////////////////////////////////
// class SqlBuffer
////////////////////////////////////////////////////////////////
class SqlBuffer : public SqlBufferBase
{
public:
  SqlBuffer(BufferType bufType)
       : SqlBufferBase(bufType)
    {};

  
  virtual SqlBuffer * castToSqlBuffer() { return this; }

  // initialize data members after allocation, given the total length
  // in bytes of the allocated SqlBuffer
  virtual void init(ULng32 in_size_in_bytes,
			       NABoolean clear = FALSE);

  // reinitialize data members; note that this version of init()
  // can only be used after a buffer has been initialized via 
  // init(unsigned long).
  virtual void init();

  // find or allocate a free tuple_descriptor with the specified length
  virtual tupp_descriptor *allocate_tuple_desc(Lng32 tup_data_size)
  {return NULL;};

  // returns whether this buffer is free
  virtual short isFree()
  {return 0;};

  // returns whether this buffer is free (reinitializes the buffer if
  // it no longer contains referenced tupps)
  short freeBuffer();

  // deallocate space from nonused tuples (noop for now)
  void compactBuffer();

  // is space for a tuple with the given length available?
  virtual short space_available(Lng32 tup_data_size);

  // change a queue_entry to GET_NOMORE before it is sent.
  NABoolean findAndCancel( queue_index pindex, 
                                      NABoolean startAtBeginning);

  /////////////////////////////////////////////////////////////
  // Methods to retrieve tupp_descriptors from this buffer.
  /////////////////////////////////////////////////////////////

  // returns maximum number of tupps in this buffer
  Lng32 getTotalTuppDescs(){return maxTuppDesc_;}

  unsigned short getTotalValidTuppDescs(){return maxValidTuppDesc_;}

  void setTotalValidTuppDescs(Lng32 val)
  {
    ex_assert(val < USHRT_MAX, "total valid tupps in buffer is larger than USHRT_MAX");
    maxValidTuppDesc_ = (unsigned short) val;
  }

  // returns the number of tupp descriptors that have been
  // processed, that is, they have been returned via one
  // of the get* calls.
  Lng32 getProcessedTuppDescs(){return tuppDescIndex_;}
    
  void setProcessedTuppDescs(unsigned short tupp_desc_num)
  {
    tuppDescIndex_ = tupp_desc_num;
  }

  // Returns the "tupp_desc_num"th tupp descriptor.
  // Returns NULL, if tupp_desc_num is greater than maxTupleDesc_.
  virtual tupp_descriptor * getTuppDescriptor(Lng32 tupp_desc_num);

  // returns the 'prev' tupp descriptor. Decrements the
  // position. Returns null, if none found.
  virtual tupp_descriptor * getPrev();
  
  // atEOTD (At End Of Tupp Descriptors)
  // returns -1, if all tupp descriptors have been returned
  short atEOTD() const
  {
    if ((tuppDescIndex_ < maxTuppDesc_) ||
        (flags_ & IGNORE_CONTROL_FLAG))
      return 0;
    else
      return -1;
  }

  // print buffer info. For debugging
  void printInfo();

  virtual void resize_tupp_desc(UInt32 tup_data_size, char *dataPointer) { };

  NABoolean varLenRows() { return (flags_ & VAR_LEN_ROWS) != 0; };
  
  void setVarLenRows(NABoolean v)
  {(v ? flags_ |= VAR_LEN_ROWS : flags_ &= ~VAR_LEN_ROWS);}

  virtual SqlBuffer::moveStatus 
  moveInSendOrReplyData(NABoolean isSend,   // TRUE = send
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
			ex_expr_base * expr = NULL, 
			atp_struct * atp1 = NULL,
			atp_struct * workAtp = NULL, 
			atp_struct * destAtp = NULL, 
			unsigned short tuppIndex = 0,
			NABoolean doMoveStats = FALSE, // if TRUE & statsArea
			                              // is passed in,  
			                              // move in stats
			ExStatisticsArea * statsArea = NULL,
			tupp_descriptor ** statsDesc = NULL,
			NABoolean useExternalDA = FALSE,
			NABoolean callerHasExternalDA = FALSE,
			tupp_descriptor * defragTd = NULL
#if (defined(_DEBUG))
			,ex_tcb * tcb = NULL  // for debuggin
#endif
			,NABoolean noMoveWarnings = FALSE
                        );
  
  
  virtual NABoolean moveOutSendOrReplyData(NABoolean isSend,
					   void * currQState,
					   tupp &outTupp,
					   ControlInfo ** controlInfo,
					   ComDiagsArea ** diagsArea,
					   ExStatisticsArea ** statsArea = NULL,
					   Int64 * numStatsBytes = NULL,
					   NABoolean noStateChange = FALSE,
					   NABoolean unpackDA = FALSE,
					   CollHeap * heap = NULL);
       
  
  ControlInfo * getControlInfo() {return &srControlInfo_;};

  // To get more info if sql_buffer received twice. 
  void setSignature();
  inline void setInvalidSignature()
  {
    signature_ = (Int64) INVALID_SIGNATURE;
  }
  inline Int64 getSignature(void) const
  {
    return signature_;
  }
  NABoolean checkSignature(const Int32 nid, const Int32 pid,
                           Int64 *signature, const Int32 action);

  ////////////////////////////////////////////////////////////////////////
  // SEND* requests: These flags set by sender which sends input requests
  //                 (down request entries).
  // SEND_SAME_REQUEST: 
  //        Each down entry has the same queue downState. One control
  // entry is moved to buffer followed by multiple data entries. If
  // any queue downState is 'different' than the previous one, then
  // a new controlInfo tuple is moved in. Multiple input entries with
  // the same parentIndex could happen for cases when rows are being
  // sent from top, ex. for an "INSERT...SELECT", the tupleFlow operator
  // sends multiple rows belonging to the same INSERT...SELECT request.
  //
  // SEND_CONSECUTIVE_REQUEST:
  //   Each input request has a downState that is different in only
  // the parentIndex and each subsequent parentIndex is one greater
  // than the previous one. Ex. inputs to right child of a nested
  // join.
  // 
  // REPLY* requests: The flags are set by the replier which returns
  //                  reply rows (up reply entries).
  // REPLY_GET_UNIQUE_REQUEST: 
  //   Each reply entry either returns exactly one row or no rows.
  // And each up entry's parentIndex is one greater than the previous
  // one. One control tuple is moved followed by data tuples, as long
  // as their parentIndex are one greater than the previous one. An
  // empty data tuple of length zero is put in for those requests that
  // do not return any rows.
  //                    
  // REPLY_GET_SUBSET_REQUEST:
  //   Each input entry has multiple reply data rows. Each reply row
  // has the same parentIndex as input request but the matchNo value
  // is one greater than the previous one.
  // 
  // REPLY_VSBB_INSERT_REQUEST:
  //   For buffered inserts, each input buffer sent from exe to eid
  // contains one 'row'. This 'row' is actually a vsbb insert buffer
  // (class SqlBuffer) and contains multiple data rows. The reply to
  // these input 'rows' contain the number of inserted rows in the
  // matchNo field of the upState. 
  //  
  ////////////////////////////////////////////////////////////////////////
  enum SRFlags
  {
    SEND_SAME_REQUEST = 0, SEND_CONSECUTIVE_REQUEST,
    SEND_VSBB_WITH_EOD,
    REPLY_GET_UNIQUE_REQUEST, REPLY_GET_SUBSET_REQUEST,
    REPLY_VSBB_INSERT_REQUEST, DEFAULT_
  };

  SRFlags srFlags() {return (SRFlags)srFlags_;};
  void setSRFlags(SRFlags srf)
  {
    srFlags_ = (unsigned short)srf;
  };

  Lng32 getFreeSpace() { return freeSpace_; }

  virtual NABoolean hasEnoughFreeSpace( Lng32 neededSpace)
  {
    return FALSE;
  }

  virtual unsigned short getNumInvalidTuplesUptoNow();
    
  virtual void setNumInvalidTuplesUptoNow(Lng32 val);

  virtual void incNumInvalidTuplesUptoNow();

  virtual void decNumInvalidTuplesUptoNow();

  virtual void advanceToNextValidTuple();

  void sameExecution(bool s) {
    if (s) 
      flags_ |= SAME_EXECUTION;
  }

  inline bool isSameExecution() const { 
    if (flags_ & SAME_EXECUTION) 
      return true;
    else
      return false;
  }

  void firstExecution(bool s) {
    if (s) 
      flags_ |= FIRST_EXECUTION;
  }

  inline bool isFirstExecution() const { 
    if (flags_ & FIRST_EXECUTION) 
      return true;
    else
      return false;
  }
   
#ifdef DO_INTEGRITY_CHECK
  // Redefine from no-op in base class to a real integrity check
  // for SqlBuffer and derived classes.
  virtual void integrityCheck( NABoolean ignore_omm_check=FALSE);
#else
  // Cannot afford to check reply_buffer for release code.
#endif

  enum SIGNATURE_MODE
  {
    INITIAL_SIGNATURE, // not used yet
    INVALID_SIGNATURE = -5555555
  };

protected:
  Lng32		 freeSpace_;         // free space in the buffer
  ULng32  maxTuppDesc_;       // max tupp desc allocated. Some of
                                     // them may have a reference count of 0.
  // used to read all tuple descriptors from the buffer.
  // See position() and getNext().
  ULng32  tuppDescIndex_;

  // send/receive flags will be unioned with maxValidTuppDesc
  // as maxValidTuppDesc is used only in VSBBInsert node between the
  // from the SETUP state to DONE state. This data member is reset in the
  // DONE state if it was ever used as maxValidTuppDesc_
  union {
  //send/receive flags.
  unsigned short srFlags_;

  // number of valid rows in this buffer. Rows become invalid if they 
  // raise a setup error during non-atomic insert
  unsigned short maxValidTuppDesc_;
  };

  // See IGNORE_CONTROL_FLAG, CURR_HAS_EDA etc.
  short flags_;

  // To get more info if sql_buffer received twice. 
  Int64 signature_;

  // this variable is used to keep track of the control info associated
  // with the 'current' send or reply data tuple that is being added
  // or accessed. See methods moveInSendOrReplyData and moveOutSendOrReplyData
  // for details on how this variable is used.
  ControlInfo srControlInfo_;
  

  virtual tupp_descriptor * tupleDesc(Int32 i)
  { return 0; }


private:
  enum PROCEED_OR_DEFER
  {
    DEFER_MORE_TUPPS, // related to REPLY_VSBB_INSERT_REQUEST
    PROCEED_MORE_TUPPS
  };

Int32 setupSrControlInfo( NABoolean isSend, tupp_descriptor *tuppDesc );

};

/////////////////////////////////////////////////////////////////////
// class SqlBufferNormal
//
// An SqlBuffer is a C++ class with variable length. Instead of
// using a constructor, it is allocated with the method
// sql_buffer_pool::addBuffer(). An SQL buffer looks like this:
//
//    +-----------------------------+
//    | SqlBuffer data members     |
//    | (# of tupp_descriptors,     |
//    | data offset, free space,    |
//    | total length, ...)          |
//    +-----------------------------+
//    | tupp data 1                 |
//    +-----------------------------+
//    | tupp data 2                 |
//    +-----------------------------+
//    | ...                         |
//    +-----------------------------+
//    | tupp data n                 |
//    +-----------------------------+
//    |                             |
//    | free space                  |
//    |                             |
//    +-----------------------------+
//    | tupp_descriptor n           |
//    +-----------------------------+
//    | ...                         |
//    +-----------------------------+
//    | tupp_descriptor 2           |
//    +-----------------------------+
//    | tupp_descriptor 1           |
//    +-----------------------------+
//
// Tupp descriptors are allocated from the end of the buffer. The 
// data space is allocated from the top of the buffer to facilitate
// resizing of the tupp data. 
//
// Data for each tupp_descriptor begins on an 8 byte boundary. Data
// for the tupp_descriptors does not necessarily have to be laid out
// in the way shown, and not all tupp_descriptors have to have the
// same data length. There may be holes in the tupp_descriptors and
// in the tupp data. Holes are not counted as free space.
//
//
////////////////////////////////////////////////////////////////////
class SqlBufferNormal : public SqlBuffer
{
  friend class sql_buffer_pool;
  friend class SqlTable;
  friend class SimSqlTable;

#ifdef SQL_BUFFER_SIGNATURE
  friend class ExEIDRootTcb;
#endif

public:

  SqlBufferNormal()
       : SqlBuffer(NORMAL_)
    {
    };

  virtual Lng32 getClassSize(){return sizeof(SqlBufferNormal);};

  // initialize data members after allocation, given the total length
  // in bytes of the allocated sql_buffer
  virtual void init(ULng32 in_size_in_bytes,
			       NABoolean clear = FALSE);

  // reinitialize data members; note that this version of init()
  // can only be used after a buffer has been initialized via 
  // init(unsigned long).
  virtual void init();

  // find or allocate a free tuple_descriptor with the specified length
  virtual tupp_descriptor *allocate_tuple_desc(Lng32 tup_data_size);

  // add a new tuple descriptor to the end of the buffer
  virtual tupp_descriptor *add_tuple_desc(Lng32 tup_data_size);

  // remove the tuple desc with the highest number from the buffer
  virtual void remove_tuple_desc();

  // returns whether this buffer is free
  virtual short isFree();

  // total size in bytes
  virtual ULng32 get_used_size()
                            { return (sizeInBytes_ - freeSpace_); }

  // number of bytes to be sent across processes. For SqlBufferDense,
  // this method is redefined to return the actual number of bytes to
  // be sent.
  virtual ULng32 getSendBufferSize()
    { return get_buffer_size(); }

  // converts all pointers in tuple descriptors to offset relative
  // to the beginning of sql_buffer. NOTE: sql_buffer is not derived
  // from the ExGod class but has similar pack and unpack methods.
  virtual void pack();

  virtual void unpack();

  // A method to call before unpacking to verify the internal
  // consistency of a packed object. The main concern is to make sure
  // that after we convert offsets to pointers, we are not referencing
  // memory outside the bounds of the incoming message
  // buffer. Currently verify() is only called by the ExUdrTcb class
  // when it receives a data reply from the non-trusted UDR server.
  virtual NABoolean verify(ULng32 maxBytes) const;

  /////////////////////////////////////////////////////////////
  // Methods to retrieve tupp_descriptors from this buffer.
  /////////////////////////////////////////////////////////////

  // positions to the first tupp descriptor in the buffer.
  virtual void position();

  // positions to the "tupp_desc_num"th tupp descriptor.
  virtual void position(Lng32 tupp_desc_num);
  
  // returns the 'next' tupp descriptor. Increments the
  // position. Returns null, if none found.
  virtual tupp_descriptor * getNext();

  // returns the 'prev' tupp descriptor. Decrements the
  // position. Returns null, if none found.
  virtual tupp_descriptor * getPrev();

  // returns the 'current' tupp descriptor. Does not increment
  // position. Returns null, if none found.
  virtual tupp_descriptor * getCurr() { return getCurrNormal(); };
  tupp_descriptor * getCurrNormal() {
    if (tuppDescIndex_ < maxTuppDesc_)
      return tupleDesc((Int32)tuppDescIndex_);
    else
      return 0;
  }

  // advances the current tupp descriptor
  virtual void advance() { advanceNormal(); };
  void advanceNormal() {
    tuppDescIndex_++;
  }

  // Returns the "tupp_desc_num"th tupp descriptor.
  // Returns NULL, if tupp_desc_num is greater than maxTupleDesc_.
  virtual tupp_descriptor * getTuppDescriptor(Lng32 tupp_desc_num);
  
  virtual unsigned short getNumInvalidTuplesUptoNow();
    
  virtual void setNumInvalidTuplesUptoNow(Lng32 val);

  virtual void incNumInvalidTuplesUptoNow();

  virtual void decNumInvalidTuplesUptoNow();

  // print buffer info. For debugging
  void printInfo();

  virtual void resize_tupp_desc(UInt32 tup_data_size, char *dataPointer);

  //////////////////////////////////////////////////

  
  virtual SqlBufferNormal * castToSqlBufferNormal() { return this; }

private:
    ULng32 	dataOffset_; // offset from where data space is to be
                             // to be allocated.  
    Lng32 tupleDescOffset_;
                             // this is also the end of the buffer as
                             // tupp descs are allocated in reverse order. 

  // number of invalid rows upto (not including) the current position of buffer.
  // intialized to NO_INVALID_TUPLES. The value 0 means that this buffer
  // has invalid tuples, but not before the current position. 
  unsigned short numInvalidTuplesUptoCurrentPosition_;

  unsigned short normalBufFlags_;

  virtual tupp_descriptor * tupleDesc(Int32 i)
    { // tuple descs are allocated in reverse order, from the end of the buffer. 
      return &((tupp_descriptor*)((char *)this + tupleDescOffset_))[-(i+1)];
    };

};


/////////////////////////////////////////////////////////////////////////
// class SqlBufferDense
/////////////////////////////////////////////////////////////////////////
class SqlBufferDense : public SqlBuffer
{
  friend class SqlBuffer;

public:
  
  SqlBufferDense();

  virtual Lng32 getClassSize(){return sizeof(SqlBufferDense);};

  // initialize data members after allocation, given the total length
  // in bytes of the allocated sql_buffer
  virtual void init(ULng32 in_size_in_bytes,
			       NABoolean clear = FALSE);

  // reinitialize data members; note that this version of init()
  // can only be used after a buffer has been initialized via 
  // init(unsigned long).
  virtual void init();

  // find or allocate a free tuple_descriptor with the specified length
  virtual tupp_descriptor *allocate_tuple_desc(Lng32 tup_data_size);

  // add a new tuple descriptor to the end of the buffer
  virtual tupp_descriptor *add_tuple_desc(Lng32 tup_data_size);

  // remove the tuple desc with the highest number from the buffer
  virtual void remove_tuple_desc();

  // returns whether this buffer is free
  virtual short isFree();

  // positions to the first tupp descriptor in the buffer.
  virtual void position();

  // positions to the "tupp_desc_num"th tupp descriptor.
  virtual void position(Lng32 tupp_desc_num);

  // returns the 'next' tupp descriptor. Increments the
  // position. Returns null, if none found.
  virtual tupp_descriptor * getNext();

  // returns the 'prev' tupp descriptor. Decrements the
  // position. Returns null, if none found.
  virtual tupp_descriptor * getPrev();

  // returns the 'current' tupp descriptor. Does not increment
  // position. Returns null, if none found.
  virtual tupp_descriptor * getCurr() { return getCurrDense(); };
  tupp_descriptor * getCurrDense() {
    if (tuppDescIndex_ < maxTuppDesc_)
      return  currTupleDesc()->tupleDesc();
    else
      return NULL;
  }

  // advances the current tupp descriptor
  virtual void advance() { advanceDense(); };
  void advanceDense() {
    tuppDescIndex_++;
    currTupleDesc() = getNextTupleDesc(currTupleDesc());
  }

  // Returns the "tupp_desc_num"th tupp descriptor.
  // Returns NULL, if tupp_desc_num is greater than maxTupleDesc_.
  virtual tupp_descriptor * getTuppDescriptor(Lng32 tupp_desc_num);

  virtual ULng32 get_used_size()
  { return getSendBufferSize(); }

  virtual ULng32 getSendBufferSize()
    {
      // dataOffset_ points to the first free byte in this buffer.
      //return ((char *)lastTupleDesc() - (char *)this);

      // must be unpacked of lastTupleDesc() will not be a valid address.
      // return the total buffer size in this case.
      // Could also do a temporary unpack of lasttupledesc and return it.
      if ((packed()) && (lastTupleDesc()))
	return get_buffer_size();

      if (lastTupleDesc())
	return 
	  (Lng32)
	  ((char *)(lastTupleDesc()->tupleDesc()->getTupleAddress()
		 + ROUND8(lastTupleDesc()->tupleDesc()->getAllocatedSize()))
	  - (char *)this);
      else
	return   (Lng32)((char *)firstTupleDesc() - (char *)this);

    }

  virtual unsigned short getNumInvalidTuplesUptoNow();
    
  virtual void setNumInvalidTuplesUptoNow(Lng32 val);

  virtual void incNumInvalidTuplesUptoNow();

  virtual void decNumInvalidTuplesUptoNow();

  moveStatus moveInVsbbEOD(void * down_q_state);

  virtual void pack();

  virtual void unpack();

  
  virtual SqlBufferDense * castToSqlBufferDense() { return this; };

  virtual void resize_tupp_desc(UInt32 tup_data_size, char *dataPointer);

private:
  TupleDescInfo * lastTupleDesc_;
  TupleDescInfo * currTupleDesc_;
  // number of invalid rows upto (not including) the current position of buffer.
  // intialized to NO_INVALID_TUPLES. The value 0 means that this buffer
  // has invalid tuples, but not before the current position. 
  unsigned short numInvalidTuplesUptoCurrentPosition_;

  char filler_[2];


  TupleDescInfo * firstTupleDesc() 
  {
    return (TupleDescInfo*)((char *)this + ROUND8(sizeof(*this))); 
  }


  TupleDescInfo * &lastTupleDesc() { return lastTupleDesc_; };

  TupleDescInfo * &currTupleDesc() { return currTupleDesc_; };


  virtual tupp_descriptor * tupleDesc(Int32 i)
    {
      if (i >= maxTuppDesc_)
	return NULL;

      TupleDescInfo * tdi = firstTupleDesc();
      for (Int32 j = 0; j < i; j++)
	{
	  setNextTupleDesc(tdi, getNextTupleDesc(tdi));
	}

      return tdi->tupleDesc();
    };


  TupleDescInfo * getNextTupleDesc(TupleDescInfo * tdi) 
  { 
    if (tdi->getNextTDIOffset() > 0)
      return ((TupleDescInfo*)((char *)this + tdi->getNextTDIOffset()));
    else
      return NULL;
  };


  TupleDescInfo * getPrevTupleDesc(TupleDescInfo * tdi) 
  { 
    if (tdi->getPrevTDIOffset() > 0)
      return ((TupleDescInfo*)((char *)this + tdi->getPrevTDIOffset()));
    else
      return NULL;
  };


  void setNextTupleDesc(TupleDescInfo * tdi, TupleDescInfo * nextTdi)
  {
    if (nextTdi)
      tdi->setNextTDIOffset((char *)nextTdi - (char *)this);
    else
      tdi->setNextTDIOffset(0);
  }


  void setPrevTupleDesc(TupleDescInfo * tdi, TupleDescInfo * prevTdi)
  {
    if (prevTdi)
      tdi->setPrevTDIOffset((char *)prevTdi - (char *)this);
    else
      tdi->setPrevTDIOffset(0);
  }

};

///////////////////////////////////////////////////////////////////////////////
// class SqlBufferOltSmall
// This class is a very special purpose class. It is used for queries which
// use olt optimization. It is use to send request/data from fs to eid,
// and for reply from eid/dp2 to fs. Max one row of data for either send
// or reply.
// For send, this class is used for request and/or data.
// For reply, this class is used to receive reply with
// or without data, and with or without stats(PERTABLE or ACCUMULATED).
// For olt optimization cases where an error, warning, rowcount, or anything
// else is being sent, SqlBufferOlt class is used.
// DO NOT ADD any fields to this class. It is designed to reduce the number
// of bytes shipped between eid and fs.
///////////////////////////////////////////////////////////////////////////////
class SqlBufferOltSmall : public SqlBufferHeader
{
public:
  
  SqlBufferOltSmall()
    : SqlBufferHeader(OLT_SMALL_)
  {
    str_pad(filler_, 2, '\0');
  };

  
  void init()
  {
    SqlBufferHeader::init();
  };

  
  moveStatus moveInSendData(ULng32 projRowLen,
			    ex_expr_base * expr, 
			    atp_struct * atp1,
			    atp_struct * workAtp, 
			    unsigned short tuppIndex);

  
  NABoolean moveOutSendData(tupp &outTupp, ULng32 returnedRowLen);

  
  NABoolean moveOutReplyData(void * currQState,
			     tupp &outTupp,
			     ULng32 returnedRowLen,
			     ComDiagsArea ** diagsArea,
			     ExStatisticsArea ** statsArea,
			     Int64 * numStatsBytes);

  
  NABoolean sendData() { return (sendFlags() & SEND_DATA_) != 0; };
  
  void setSendData(NABoolean v)
  {(v ? sendFlags() |= SEND_DATA_ : sendFlags() &= ~SEND_DATA_);}

  
  NABoolean replyData() { return (replyFlags() & REPLY_DATA_) != 0; };
  
  void setReplyData(NABoolean v)
  {(v ? replyFlags() |= REPLY_DATA_ : replyFlags() &= ~REPLY_DATA_);}

  
  NABoolean replyRowAffected() { return (replyFlags() & REPLY_ROW_AFFECTED_) != 0; };
  
  void setReplyRowAffected(NABoolean v)
  {(v ? replyFlags() |= REPLY_ROW_AFFECTED_ : replyFlags() &= ~REPLY_ROW_AFFECTED_);}

  
  NABoolean replyStats() { return (replyFlags() & REPLY_STATS_) != 0; };
  
  void setReplyStats(NABoolean v)
  {(v ? replyFlags() |= REPLY_STATS_ : replyFlags() &= ~REPLY_STATS_);}

  NABoolean replyStatsOnly() 
  { return (replyFlags() & REPLY_STATS_ONLY_) != 0; };
  
  void setReplyStatsOnly(NABoolean v)
  {(v ? replyFlags() |= REPLY_STATS_ONLY_ : replyFlags() &= ~REPLY_STATS_ONLY_);}

  
  char * sendDataPtr()
  {
    return (sendData() ? (char *)((char *)this+sizeof(SqlBufferOltSmall)) : NULL);
  };

private:
  enum SendFlags
  {
    SEND_DATA_ =         0x01
  };

  enum ReplyFlags
  {
    REPLY_DATA_ =         0x01,
    REPLY_ROW_AFFECTED_ = 0x02,
    REPLY_STATS_        = 0x04,
    REPLY_STATS_ONLY_   = 0x08
  };

  char filler_[2];
};

//////////////////////////////////////////////////////////////
// class SqlBufferOlt
//////////////////////////////////////////////////////////////
class SqlBufferOlt : public SqlBufferBase
{
private:
  enum OltBufFlags
  { 
    STATS_ = 0x01, CONTAINS_OLT_SMALL_ = 0x02, DATA_PROCESSED_ = 0x04
  };

  enum Contents
  {
    NOTHING_YET_ = 0,
    ERROR_, DATA_, DATA_WARNING_, 
    NODATA_, NODATA_ROWAFFECTED_, NODATA_ROWCOUNT_, NODATA_WARNING_,
    NODATA_ROWAFFECTED_WARNING_, NODATA_ROWCOUNT_WARNING_, STATSONLY_
  };

public:
  SqlBufferOlt()
       : SqlBufferBase(SqlBufferBase::OLT_),
	 oltBufFlags_(0)
  {};

  virtual Lng32 getClassSize(){return sizeof(SqlBufferOlt);};

  // initialize data members after allocation, given the total length
  // in bytes of the allocated sql_buffer
  virtual void init(ULng32 in_size_in_bytes,
			       NABoolean clear = FALSE);

  // reinitialize data members; note that this version of init()
  // can only be used after a buffer has been initialized via 
  // init(unsigned long).
  virtual void init();

  // converts all pointers in tuple descriptors to offset relative
  // to the beginning of sql_buffer. NOTE: sql_buffer is not derived
  // from the ExGod class but has similar pack and unpack methods.
  virtual void pack();

  virtual void unpack();

  // add a new tuple descriptor to the end of the buffer
  virtual tupp_descriptor *add_tuple_desc(Lng32 tup_data_size);

  // remove the tuple desc with the highest number from the buffer
  virtual void remove_tuple_desc();

  
  virtual moveStatus 
  moveInSendOrReplyData(NABoolean isSend,   // TRUE = send
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
			ex_expr_base * expr = NULL, 
			atp_struct * atp1 = NULL,
			atp_struct * workAtp = NULL, 
			atp_struct * destAtp = NULL, 
			unsigned short tuppIndex = 0,
			NABoolean doMoveStats = FALSE, // if TRUE & statsArea
			                              // is passed in,  
			                              // move in stats
			ExStatisticsArea * statsArea = NULL,
			tupp_descriptor ** statsDesc = NULL,
			NABoolean useExternalDA = FALSE,
			NABoolean callerHasExternalDA = FALSE,
			tupp_descriptor * defragTd = NULL
#if (defined(_DEBUG))
                        ,ex_tcb * tcb = NULL  // for debuggin
#endif
			,NABoolean noMoveWarnings = FALSE
                        );

  
  
  virtual NABoolean moveOutSendOrReplyData(NABoolean isSend,
					   void * currQState,
					   tupp &outTupp,
					   ControlInfo ** controlInfo,
					   ComDiagsArea ** diagsArea,
					   ExStatisticsArea ** statsArea = NULL,
					   Int64 * numStatsBytes = NULL,
					   NABoolean noStateChange = FALSE,
					   NABoolean unpackDA = FALSE,
					   CollHeap * heap = NULL);

  
  
  moveStatus moveInSendData(ULng32 projRowLen,
			    ex_expr_base * expr, 
			    atp_struct * atp1,
			    atp_struct * workAtp, 
			    unsigned short tuppIndex);

  
  NABoolean moveOutSendData(tupp &outTupp);

  
  moveStatus 
  moveInReplyData(NABoolean doMoveControl,
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
		  NABoolean doMoveStats, // if TRUE & statsArea
		                         // is passed in, move in stats 
		  ExStatisticsArea * statsArea,
		  tupp_descriptor ** statsDesc);

  
  NABoolean moveOutReplyData(void * currQState,
			     tupp &outTupp,
			     ComDiagsArea ** diagsArea,
			     ExStatisticsArea ** statsArea,
			     Int64 * numStatsBytes);

  
  virtual SqlBufferOlt * castToSqlBufferOlt() { return this; };

  
  NABoolean isStats() { return (oltBufFlags_ & STATS_) != 0; };
  
  void setIsStats(NABoolean v)
  {(v ? oltBufFlags_ |= STATS_ : oltBufFlags_ &= ~STATS_);}

  
  NABoolean dataProcessed() { return (oltBufFlags_ & DATA_PROCESSED_) != 0; };
  
  void setDataProcessed(NABoolean v)
  {(v ? oltBufFlags_ |= DATA_PROCESSED_ : oltBufFlags_ &= ~DATA_PROCESSED_);}

  
  NABoolean containsOltSmall() { return (oltBufFlags_ & CONTAINS_OLT_SMALL_) != 0; };
  
  void setContainsOltSmall(NABoolean v)
  {(v ? oltBufFlags_ |= CONTAINS_OLT_SMALL_ : oltBufFlags_ &= ~CONTAINS_OLT_SMALL_);}


  virtual ULng32 get_used_size()
  { return getSendBufferSize(); }

  
  Contents getContents() { return (Contents)contents_; }
  
  void setContents(Contents c) { contents_ = (unsigned char)c; }

  
  tupp_descriptor * getNextTuppDesc(tupp_descriptor * tdesc,
				    Lng32 rowlen = -1)
  {
    tupp_descriptor * nextTdesc = NULL;

    if (tdesc)
      {
	nextTdesc = 
	  (tupp_descriptor*)(tdesc->getTupleAddress() + 
			     ROUND8(tdesc->getAllocatedSize()));
      }
    else
      {
	nextTdesc = 
	  (tupp_descriptor*)((char *)this+ROUND8(sizeof(SqlBufferOlt)));

	if (rowlen >= 0)
	  {
	    sizeInBytes_ = ROUND8(sizeof(SqlBufferOlt));
	  }
      }
    
    if (rowlen < 0)
      nextTdesc->setRelocatedAddress(0);
    else
      {
	nextTdesc->init(rowlen, 0, 
			(char *)nextTdesc+
				ROUND8(sizeof(tupp_descriptor)));
	sizeInBytes_ += ROUND8(sizeof(tupp_descriptor)) + ROUND8(rowlen);
      }
    
    return nextTdesc;
  }

  char * getSendDataPtr()
  {
    return (char *)this+ROUND8(sizeof(SqlBufferOlt));
  }

  virtual ULng32 getSendBufferSize()
    { 
      return sizeInBytes_;
    }

  char * getReplyDataPtr()
  {
    if (containsOltSmall())
      {
	if ((dataProcessed()) && (NOT isStats()))
	  return (char *)this+ROUND8(sizeof(SqlBufferOlt)) + 
			 ROUND8(sizeof(SqlBufferOltSmall)) - 
			 sizeof(SqlBufferHeader);
	else
	  return (char *)this+ROUND8(sizeof(SqlBufferOlt));
      }
    else
      {
	// on NT, skip the vtbl ptr. It is at the beginning of the class.
	return (char *)this + sizeof(char*);
      }
  }

  virtual ULng32 getReplyBufferSize()
    { 
      if (containsOltSmall())
	return sizeInBytes_;

      // on NT, skip the vtbl ptr. It is at the beginning of the class.
     return (sizeInBytes_ - sizeof(char*));
    }

  // is space for a tuple with the given length available?
  virtual short space_available(Lng32 tup_data_size)
  {
    return -1;
  }


private:    
  unsigned char oltBufFlags_;
  unsigned char contents_;
  short filler_;
};

/*
definitions moved to sql_buffer_size.h

static unsigned long SqlBufferGetTuppSize(
     long recordLength = 0,
     SqlBufferHeader::BufferType bufType = SqlBufferHeader::NORMAL_)
{
  long sizeofTuppDescriptor = 
    ((bufType == SqlBufferHeader::DENSE_) ? ROUND8(sizeof(TupleDescInfo))
     : sizeof(tupp_descriptor));
  
  return ROUND8(recordLength) + sizeofTuppDescriptor;
  
}

// a static method used to determine the needed buffer length if a
// given number of records with a given length are to be stored


static unsigned long SqlBufferNeededSize(long numTuples, 
					 long recordLength,
					 SqlBufferHeader::BufferType bufType)
{
  // Return the header size plus the size of any tuple descriptors
  // beyond the first (which is included in the header) plus the
  // size for the aligned data.
  long headerSize = 0;
  switch (bufType)
    {
    case SqlBufferHeader::DENSE_: 
      headerSize = sizeof(SqlBufferDense);
      break;

    case SqlBufferHeader::OLT_:
      headerSize = sizeof(SqlBufferOlt);
      break;

    case SqlBufferHeader::NORMAL_:
      headerSize = sizeof(SqlBufferNormal);
      break;

    default:
      headerSize = sizeof(SqlBufferNormal);
      break;
    }
  
  headerSize = ROUND8(headerSize);
  
  return (headerSize +
	  (numTuples * SqlBufferGetTuppSize(recordLength, bufType)));
}
*/

#ifndef UDRSERV_BUILD
/////////////////////////////////////////////////////////////////////////////
// class sql_buffer pool
/////////////////////////////////////////////////////////////////////////////
class sql_buffer_pool : public ExGod
{
public:

  // allocate a buffer pool from a space object (everything gets
  // deallocated when the space object gets deleted) and pre-allocate
  // numberOfBuffers buffers of size defaultBufferSize
  sql_buffer_pool(Lng32 numberOfBuffers, 
			     Lng32 defaultBufferSize,
			     CollHeap * space,
			     SqlBufferBase::BufferType bufType
			     = SqlBufferBase::NORMAL_);
  
  ~sql_buffer_pool();
  
  // add a new sql_buffer to the pool by specifying its total size or the
  // number of tupp_descriptors and their length
  SqlBufferBase * addBuffer(Lng32 totalBufferSize, bool failureIsFatal = true);
  SqlBufferBase * addBuffer(Lng32 totalBufferSize, 
			    SqlBufferBase::BufferType bufType,
			    bool failureIsFatal = true);

  // this method allocates one buffer but doesn't add it to pool.
  static SqlBufferBase * addBuffer(Lng32 totalBufferSize, 
				   SqlBufferBase::BufferType bufType,
				   CollHeap * space);

  // gets an empty buffer with at least freeSpace bytes of free space or
  // returns NULL if no free buffer found
  SqlBufferBase * get_free_buffer(Lng32 freeSpace);

  // initializes tp to point to a new tupp_descriptor in the pool
  // that has a data length of tupDataSize
  // RETURNS: -1, if tuple found. 0, otherwise.
  short get_free_tuple(tupp &tp, Lng32 tupDataSize, SqlBuffer **buf=NULL);
  
  // returns a new tupp_descriptor in the pool
  // that has a data length of tupDataSize.
  // Sets the reference count to 1.
  // RETURNS: tupp_descriptor, if tuple found. NULL, otherwise.
  // If buf is non-Null, also sets buf to the buffer from which 
  // the tupp_descriptor was allocated. This is used to later 
  // resize the tuple if needed.
  tupp_descriptor * get_free_tupp_descriptor(Lng32 tupDataSize, SqlBuffer **buf=NULL);
  
  // call free_buffer for all buffers in the pool
  void free_buffers();
  
  // call compactBuffer for all buffers in the pool
  void compact_buffers();

  inline Lng32 get_number_of_buffers() const
                                            { return currNumberOfBuffers_; }

  inline Lng32 get_max_number_of_buffers() const
					{ return maxNumberOfBuffers_;}

  inline void set_max_number_of_buffers(Lng32 maxnumbuf)
			{maxNumberOfBuffers_ = maxnumbuf;}

  // for debugging purposes
  void printAllBufferInfo();


  Lng32 getTotalMemorySize() { return memoryUsed_;}

  void getUsedMemorySize(UInt32 &staticMemSize,
			 UInt32 &dynMemSize);


  void resizeLastTuple(UInt32 tup_data_size, char *dataPointer);

  SqlBufferHeader::moveStatus moveIn(atp_struct *atp1,
                                     atp_struct *atp2, 
                                     UInt16 tuppIndex,
                                     Lng32 tupDataSize, 
                                     ex_expr_base *moveExpr,
                                     NABoolean addBufferIfNeeded,
                                     Lng32 bufferSize);

  SqlBufferHeader::moveStatus moveIn(atp_struct *atp,
                                     UInt16 tuppIndex,
                                     Lng32 tupDataSize,
                                     char *srcData,
                                     NABoolean addBufferIfNeeded,
                                     Lng32 bufferSize);

  NABoolean staticMode() { return ((flags_ & STATIC_MODE) != 0); };

  void setStaticMode(NABoolean v)
  { ( v ? flags_ |= STATIC_MODE : flags_ &= ~STATIC_MODE ); }
 

  Lng32 defaultBufferSize() { return defaultBufferSize_; };

  // current buffer regardles of how much space
  short currentBufferHasEnoughSpace( Lng32 tupDataSize);

  SqlBufferBase * getDefragBuffer() const
  {
    return defragBuffer_;
  }

    tupp_descriptor * addDefragTuppDescriptor(Lng32 dataSize);
  tupp_descriptor * getDefragTd()
  {
    return defragTd_;
  }

#if (defined(_DEBUG))
  static void logDefragInfo(char * txt,
                            Lng32 neededSpace,
                            Lng32 actNeededSpace,
                            Lng32 freeBuffSpace,
                            void *p,
                            Lng32 NumRowsInBuff,
                            ex_tcb * tcb = NULL);
#endif

private:
  enum Flags {STATIC_MODE = 0x0001};

  // how many buffers are currently allocated
  Lng32        currNumberOfBuffers_;

  // limit of the number of buffers in the pool
  Lng32        maxNumberOfBuffers_;

  // total amount of memory occupied by all the buffers
  Lng32        memoryUsed_;
  
  Lng32 defaultBufferSize_;

  // a linked list of sql_buffers who make up the pool
  Queue * staticBufferList_;

  // a linked list of sql_buffers who make up the pool. This
  // list is used to allocate tuples that are needed during work().
  Queue * dynBufferList_;

  // buffers are allocated from a space object
  CollHeap * space_;
  
  ULng32 flags_;

  SqlBufferBase::BufferType bufType_;

  //buffer used for defragmentation.it only needs to hold one row. 
  SqlBufferBase *defragBuffer_;
  // tuple descriptor to the row in the defrag buffer
  tupp_descriptor * defragTd_;

  // private methods
  // get the current buffer with enough free space
  inline SqlBuffer * getCurrentBuffer(Lng32 freeSpace,Int32 mustBeEmpty = 0);
  


  // try to find a buffer that is not in use by an I/O operation and that 
  // has at least freeSpace free space without performing any cleanup
  SqlBufferBase * findBuffer(Lng32 freeSpace,
				     Int32 mustBeEmpty = 0);

  // same as above, but try to free up some space if needed
  SqlBufferBase * getBuffer(Lng32 freeSpace,
				    Int32 mustBeEmpty = 0);

  SqlBufferBase * addBuffer(Queue * bufferList, 
			       Lng32 totalBufferSize,
			       SqlBufferBase::BufferType bufType,
			       bool failureIsFatal = true);
 
  SqlBufferBase * addBuffer(Lng32 numTupps, Lng32 recordLength);
  
  Queue * activeBufferList()
  {return (staticMode() ? staticBufferList_ : dynBufferList_);};

public:

  SqlBuffer * getCurrentBuffer();


};



#endif // UDRSERV_BUILD

#endif

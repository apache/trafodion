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

#include "Platform.h"

class ex_expr;
#include <sys/types.h>
#include <sys/time.h>

#include "ex_stdh.h"
//#include "fs.h"
#include "cli_stdh.h"
#include "fs/feerrors.h"

#include "Int64.h"

#ifndef __EID
//#include "arksort.h"
#endif

#ifdef __EID
// -----------------------------------------------------------------------
// Procedures that must not be called from DP2
// -----------------------------------------------------------------------

NA_EIDPROC void NAAbort(const char * filename, Int32 lineno, const char * msg)
{
}

NA_EIDPROC void NAAssert(const char * condition, const char * file_, Int32 line_)
{
}


NA_EIDPROC RETCODE Descriptor::alloc(Lng32 used_entries_)
{
  return ERROR;
}

NA_EIDPROCRETCODE Descriptor::dealloc()
{
  return ERROR;
}

NA_EIDPROC RETCODE Descriptor::getDescItem(Lng32 entry, Lng32 what_to_get, 
				Lng32 * numeric_value, char * string_value,
				Lng32 max_string_len, Lng32 * returned_len,
				Lng32 /*start_from_offset*/)
{
  return ERROR;
}

NA_EIDPROC
void Descriptor::setDescItem(Lng32 entry, Lng32 what_to_set, 
			     Lng32 numeric_value, char * string_value)
{
 
}

NA_EIDPROC
void Descriptor::setUsedEntryCount(Lng32 used_entries_)
{
  used_entries = used_entries_;

}

NA_EIDPROC
void GeneratorAbort(char *f, Int32 l, char * m)
{
}

NA_EIDPROC
void filesystem::open(char *fname, Lng32 *fnum, File::fio_open_mode open_mode)
{
}

NA_EIDPROC
void filesystem::start_subset(short fnum, short keytag,
			      char *begin_key, char *end_key,
			      ex_expr *dp2_pred,
			      char *input_values_data,
			      unsigned short reverse_scan)
{
}


NA_EIDPROC
void filesystem::get_buffer(char *data_buffer, ULng32 data_buffer_len)
{
}

NA_EIDPROC
void filesystem::await_io()
{
}

NA_EIDPROC
void filesystem::stop_subset()
{
}

NA_EIDPROC
void filesystem::close()
{
}

NA_EIDPROC
Int32 filesystem::status()
{
}

NA_EIDPROC
void filesystem::delete_buffer(char * key_buffer)
{
}

NA_EIDPROC
void filesystem::update_buffer(char * key_buffer, char * data_buffer,
			       rec_project_struct * datalist)
{
}

NA_EIDPROC
void filesystem::insert_buffer(char * data_buffer, ULng32 data_buffer_len)
{
}
#endif


#ifndef __EID
// LCOV_EXCL_START
// -----------------------------------------------------------------------
// DP2 procedures (stub out if this is not a compile for EID)
// -----------------------------------------------------------------------
Int16 DP2_EXECUTOR_POSITION ( char  * Low_Key_Addr,
		     	      Int16 * Low_Key_Excluded,
			      char  * High_Key_Addr,
			      Int16 * High_Key_Excluded,
			      Int32   Key_Length,
			      Int16   Operation,
			      char  * Next_Low_Key_Addr,
			      char  * Next_High_Key_Addr,
			      Int16   Begin_Range_Flags,
			      Int16   Reverse_Flag,
			      Int16   numIoBlocks
                            , Int16 * partOp
			    ){return 0;}

Int16 DP2_EXECUTOR_POSITION_UNIQUE (
				    char * Key_Addr,
				    Int32  Key_Length,
				    Int16  Operation,
				    char * Next_Low_Key_Addr,
				    char * Next_High_Key_Addr,
				    Int16  numIoBlocks
			           ){return 0;}

Int16 DP2_EXECUTOR_POSITION_SAMPLE (
				    Int32  percentage,
				    Int16  operation,
				    Int16  Begin_Range_Flag
				    ){return 0;}

Int16 DP2_EXECUTOR_POSITION_CSAMPLE (
				     Int32  percentage,
				     Int32  cluster_size,
				     Int16  operation,
				     Int16  Begin_Range_Flag
				    ){return 0;}

Int16 DP2_EXECUTOR_FETCH (
			  void  ** Row_Addr,
			  Int32  * Row_Length,
			  char   * syskey_addr,
			  Int16    Operation,
			  Int16  * Request_Status,
			  void  ** LastRowAddr,           
			  Int32  * LastRowLength 
			  ){return 0;}

Int16 DP2_EXECUTOR_FETCH_BLOCK (
			  void  ** Block_Addr,
			  Int32  * Block_Length,
			  Int32  * Num_Recs_In_Block,
			  Int32  * Start_Recnum,
			  Int16    Operation,
			  Int16  * Request_Status
			  ){return 0;}

Int16 DP2_EXECUTOR_FETCH_ROW (Int32    recnum,
			      Int32    blocklen,
			      void   * buffer,
			      void  ** record,
			      Int32  * reclen,
			      Int16  * request_status){return 0;};

Int16 DP2_EXECUTOR_LOCK_FILE (
                              void   * /*Lock_Flags_Struct*/ Lock_Flags,
			      Int16    Operation,
			      char   * Consumed_Key_Pointer_Addr,
			      Int32    Consumed_Key_Length,
			      Int16  * Request_Status
			      ){return 0;}

Int16 DP2_EXECUTOR_ROW_SELECTED (
				 char  * Key_Addr,
				 Int32   Key_Length,
				 Int16   Operation,
				 Int16   End_Of_Range,
				 void  * /*Lock_Flags_Struct*/ Lock_Flags,
				 Int16 * Request_Status,
				 void  * Before_Image_Addr,
				 Int32   Before_Image_Length,
				 void  * After_Image_Addr,
				 Int32   After_Image_Length,
				 void  * /*MFMap_STRUCT*/ mfMap,
                                 Int16   Op_Type   // BertBert
				 ){return 0;}

Int16 DP2_EXECUTOR_ROW_NOT_SELECTED (
				     char   * Key_Addr,
				     Int32    Key_Length,
				     Int16    Operation,
				     Int16    End_Of_Range,
				     void   * /*Lock_Flags_Struct*/ Lock_Flags,
				     Int16  * Request_Status
				     ){return 0;}

// Memory Allocation/Deallocation
Int16 DP2_EXECUTOR_ADD_MEMORY (
			       Int32    Memory_Size,
			       void  ** Returned_Memory_Addr
			       ){return 0;}

Int16 DP2_EXECUTOR_DROP_MEMORY (
				void  * Drop_Memory_Addr
			       ){return 0;}

// reply to filesystem with reply buffer
Int16 DP2_EXECUTOR_REPLY (
			  Int16   Reply_Error,
			  void  * Reply_Buffer_Addr,
			  Int32   Reply_Buffer_Length
			  ){return 0;}

Int16 DP2_EXECUTOR_INSERT (
			   char   * Key_Addr,
			   Int32    Key_Length,
			   void   * Row_Addr,
			   Int32    Row_Length,
			   void   * /*Lock_Flags_Struct*/ Lock_Flags,
			   void   * /*Insert_Control_Flags_Struct*/ Insert_Control_Flags,
			   Int16  * Request_Status,
			   char   * Range_Protector_Key_Addr,
			   Int32  * Range_Protector_Key_Length
			   ){return 0;}

Int16 DP2_EXECUTOR_INSERT_VSBB (
				void   * /*Lock_Flags_Struct*/ Lock_Flags,
				void   * VSBB_Insert_Buffer,
				Int32    VSBB_Buffer_Length,
				char   * Range_Protector_Key_Addr,
				Int16    Insert_Anywhere,
				Int16    Insert_At_End,
				Int16  * Request_Status
				){return 0;}

Int16 DP2_EXECUTOR_SIDETREE_INSERT (
                                   void   * VSBB_Insert_Buffer,
				   Int32    VSBB_Buffer_Length,
				   Int16  * Request_Status
				   ){return 0;}
				   
Int16 DP2_EXECUTOR_SIDETREE_SETUP (
				  Int16  * Request_Status
				  ){return 0;}

Int16 DP2_EXECUTOR_SIDETREE_COMMIT (
				   Int16  * Request_Status
				   ){return 0;}

Int16 DP2_EXECUTOR_VSBB_NAK (
			    char * Range_Protector_Key_Addr,
			    Int32  Range_Protector_Key_Length
			    ){return 0;}

Int16 DP2_EXECUTOR_DELETE_RANGE(
				void   * Scan_Expr_Pointer,
				void   * /*Lock_Flags_Struct*/ Lock_Flags,
				char   * Last_Begin_Key_Addr,
				Int32  * Last_Begin_Key_Length,
				Int16  * Request_Status
				){return 0;}

Int16 DP2_EXECUTOR_GET_OVERFLOW_DATA(
				     void * Buffer_Addr, 
				     Int32  Buffer_Length
				     ){return 0;}

Int32 DP2_EXECUTOR_SESSION_NID() {return 0;}
Int32 DP2_EXECUTOR_SESSION_PID() {return 0;}

Int16 DP2_EXECUTOR_SWITCH_CONTEXT(Int16   switchSessionFlag,
				  Int64   OCBID,
				  Int16  ocbindex,
				  void ** RCB
                                , void ** Row_Hiding_Predicate
				  ){return 0;}


Int16 DP2_EXECUTOR_SET_WAITFORROWS(void){return 1;}
void DP2_EXECUTOR_DISCARD_DELTA_KEY(Int32 DP2_key_index){return;}
void DP2_EXECUTOR_DISC_DK_ATCOMMIT(Int32 DP2_key_index){return;}
Int16 DP2_EXECUTOR_GET_DELTA_KEY(char  ** key_addr,
                                 Int32  * key_len,
                                 Int32  * DP2_key_index){return 0;}
Int16 DP2_EXECUTOR_DELTA_SCAN_STATUS(Int16 delta_scan_active,
                                     Int16 regular_scan_is_finished){return 0;}
void DP2_EXECUTOR_DELTA_SCAN_INIT(Int16  streaming,
                                  Int16  destructive,
                                  Int16  ordered, 
                                  Int32  stream_timeout,
                                  Int32  rowset_size){return;}
void DP2_EXECUTOR_INSERT_DK_IN_SSCB(char * in_key, Int32 in_key_len){return;}
void DP2_EXECUTOR_UPDATE_OP_IN_SSCB(Int16  operation){return;}
Int16 DP2_EXECUTOR_CHECK_DK_X_LOCKED(Int32 dp2_key_index){return TRUE;}
void DP2_EXECUTOR_INSERT_DK_UNCOMMIT(char  * low_key,
                                     Int32   low_key_excluded,
		                     char  * high_key,
                                     Int32   high_key_excluded,
	                             Int32   key_len,
                                     char  * anchor_key){};
Int16 DP2_EXECUTOR_REPOSITION_NEEDED(void){return TRUE;}
Int64 DP2_EXECUTOR_GET_SYSKEY(Int64  prev_syskey){return 0;}
void DP2_EXECUTOR_GET_STATS(Int32 * stats,
                            Int32   values_requested, 
                            Int32 * values_returned){return;}

void DP2_EXECUTOR_SESSION_STATS(void * stats) {return;}

Int16 DP2_EXECUTOR_QUERYID_STATS(void * stats) {return 0;}

Int16 DP2_EXECUTOR_MXBUFFER_STATS(void * stats) {return -1;}

Int16 DP2_EXECUTOR_FETCH_UNIQUE (
                                 void   * /*Lock_Flags_Struct*/ Lock_Flags,
                                 char   * Key_Addr,
				 Int32    Key_Length,
				 void  ** Row_Addr,
				 Int32  * Row_Length,
				 void   * Unique_Expr_Addr,
				 Int16  * Request_Status
				 ){return 0;}

Int16 DP2_EXECUTOR_DELETE_UNIQUE (
                                 void   * /*Lock_Flags_Struct*/ Lock_Flags,
                                 char   * Key_Addr,
				 Int32    Key_Length,
				 void   * Unique_Expr_Addr,
				 Int16  * Request_Status
				 ){return 0;}

Int16 DP2_EXECUTOR_UPDATE_UNIQUE (
                                 void   * /*Lock_Flags_Struct*/ Lock_Flags,
                                 char   * Key_Addr,
				 Int32    Key_Length,
				 void   * Unique_Expr_Addr,
                                 void   * Updated_Row_Buffer,
				 void   * /*MFMap_STRUCT*/ mfMap,
				 Int16  * Request_Status
				 ){return 0;}

Int16 DP2_EXECUTOR_INSERT_UNIQUE (
                                 void   * /*Lock_Flags_Struct*/ Lock_Flags,
                                 char   * Key_Addr,
				 Int32    Key_Length,
				 void   * Row_Addr,
				 Int32    Row_Length,
				 void   * /*Insert_Control_Flags_Struct*/ Insert_Control_Flags,
				 void   * Unique_Expr_Addr,
				 Int16  * Request_Status
				 ){return 0;}

void DP2_EXECUTOR_COUNT_ROWS(Int64 rows_accessed, Int64 rows_selected) {}

// LCOV_EXCL_STOP
#endif /* __EID */

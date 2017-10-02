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
#include "ComVersionPrivate.h"
#include "ex_rcb.cpp"	
#include "cluster.cpp"		
#include "ex_control.cpp"		
#include "ex_ddl.cpp"		
//#include "ex_delete.cpp"
#include "ex_dp2_insert.cpp"	
#include "ex_dp2_interface.cpp"	
#include "ex_dp2_oper.cpp"		
#include "ex_dp2_range_oper.cpp"	
#include "ex_dp2_subs_oper.cpp"	
#include "ex_dp2_unique_oper.cpp"	
#include "ex_dp2_unique_lean_oper.cpp"	
#include "ex_dp2exe_root.cpp"	
#include "exdp2trace.cpp"
#include "ex_eid_stmt_globals.cpp"	
#include "ex_error.cpp"		
#include "ex_esp_frag_dir.cpp"	
#include "ex_esp_msg.cpp"		
#include "ex_ex.cpp"		
#include "ex_exe_stmt_globals.cpp"	
#include "ex_frag_inst.cpp"	
#include "ex_frag_rt.cpp"		
#include "ex_globals.cpp"		
#include "ex_god.cpp"		
#include "ex_hash_grby.cpp"	
#include "ex_hashj.cpp"		
// #include "ex_key_object.cpp"	
#include "ex_lock.cpp"		
#include "ex_mj.cpp"		
#include "ex_onlj.cpp"		
#include "ex_partn_access.cpp"	
#include "ex_queue.cpp"		
#include "ex_send_bottom.cpp"	
#include "ex_send_top.cpp"		
#include "ex_sort.cpp"		
#include "ex_sort_grby.cpp"	
#include "ex_split_bottom.cpp"	
#include "ex_split_top.cpp"	
#include "ex_stored_proc.cpp"	
#include "ex_timeout.cpp"  
#include "ex_tcb_private.cpp"	
#include "ex_tuple.cpp"		
#include "ex_tuple_flow.cpp"	
#include "ex_union.cpp"		
//#include "ex_update.cpp"		
#include "ExBitMapTable.cpp"	
#include "ExCompoundStmt.cpp"	
//#include "ExExeUtil.cpp"
#include "ExExplain.cpp"
#include "ExFirstN.cpp"	
#include "ExIar.cpp"		
#include "ExPack.cpp"		
#include "ExPackedRows.cpp"
#include "ExParLabOp.cpp"	
#include "ExScheduler.cpp"		
#include "ExSample.cpp"		
#include "ExSequence.cpp"		
#include "ExSimpleSample.cpp"	
#include "ExSimpleSqlBuffer.cpp"	
#include "ExStats.cpp"		
#include "ExTranspose.cpp"		
#include "ExUdr.cpp"		
#include "ExUdrClientIpc.cpp"		
#include "ExUdrServer.cpp"		
#include "CliMsgObj.cpp"
#include "UdrExeIpc.cpp"
#include "ExRsInfo.cpp"
#include "FixedSizeHeapManager.cpp"	
#include "hash_table.cpp"		
#include "key_Mdam.cpp"		
#include "key_range.cpp"		
#include "key_single_subset.cpp"	
#include "ex_mdam.cpp"		
#include "MdamEndPoint.cpp"	
#include "MdamInterval.cpp"	
#include "MdamIntervalIterator.cpp"	
#include "MdamIntervalList.cpp"	
#include "MdamIntervalListMerger.cpp"	
#include "MdamPoint.cpp"		
#include "MdamRefList.cpp"		
#include "MdamRefListEntry.cpp"	
#include "MdamRefListIterator.cpp"	
#include "sql_buffer.cpp"				
#include "stubs2.cpp"		

// merge join overflow support:
#include "Allocator.cpp"
#include "BufferList.cpp"
#include "BufferReference.cpp"
#include "SwapSpace.cpp"
#include "TupleSpace.cpp"
#include "ExDupSqlBuffer.cpp"

// #include "tempfile.cpp"
#include "timeout_data.cpp" 
#include "TriggerEnable.cpp"
#include "ExProbeCache.cpp"
#include "ExCancel.cpp"
#include "ExBlockingHdfsScan.cpp"


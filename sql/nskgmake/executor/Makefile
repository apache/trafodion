#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

CPPSRC := Allocator.cpp \
	BufferList.cpp \
	BufferReference.cpp \
	CliMsgObj.cpp \
	cluster.cpp \
	ex_control.cpp \
	ex_ddl.cpp \
	ex_error.cpp \
	ex_esp_frag_dir.cpp \
	ex_esp_msg.cpp \
	ex_ex.cpp \
	ex_exe_stmt_globals.cpp \
	ex_fastsort.cpp \
	ex_frag_inst.cpp \
	ex_frag_rt.cpp \
	ex_globals.cpp \
	ex_god.cpp \
	ex_hash_grby.cpp \
	ex_hashj.cpp \
	ex_key_object.cpp \
	ex_mdam.cpp \
	ex_mj.cpp \
	ex_onlj.cpp \
	ex_queue.cpp \
	ex_root.cpp \
	ex_send_bottom.cpp \
	ex_send_top.cpp \
	ex_sort.cpp \
	ex_sort_grby.cpp \
	ex_split_bottom.cpp \
	ex_split_top.cpp \
	ex_stored_proc.cpp \
	ex_tcb_private.cpp \
	ex_timeout.cpp \
	ex_transaction.cpp \
	ex_tuple.cpp \
	ex_tuple_flow.cpp \
	ex_union.cpp \
	ExBitMapTable.cpp \
	ExCompoundStmt.cpp \
	ExComTdb.cpp \
	ExDupSqlBuffer.cpp \
	ExExeUtilAll.cpp \
	ExExplain.cpp \
	ExFirstN.cpp \
	ExHbaseAccess.cpp \
	ExHbaseSelect.cpp \
	ExHbaseIUD.cpp \
	ExHbaseDDL.cpp \
	ExMeas.cpp \
	ExPack.cpp \
	ExPackedRows.cpp \
	ExProbeCache.cpp \
	ExRLE.cpp \
	ExSample.cpp \
	ExScheduler.cpp \
	ExSequence.cpp \
	ExSimpleSample.cpp \
	ExSimpleSqlBuffer.cpp \
	ExStats.cpp \
	ExTranspose.cpp \
	ExUdr.cpp \
	ExUdrClientIpc.cpp \
	ExUdrServer.cpp \
	ExRsInfo.cpp \
	FixedSizeHeapManager.cpp \
	hash_table.cpp \
	hiveHook.cpp \
	key_mdam.cpp \
	key_range.cpp \
	key_single_subset.cpp \
	MdamEndPoint.cpp \
	MdamInterval.cpp \
	MdamIntervalIterator.cpp \
	MdamIntervalList.cpp \
	MdamIntervalListMerger.cpp \
	MdamPoint.cpp \
	MdamRefList.cpp \
	MdamRefListEntry.cpp \
	MdamRefListIterator.cpp \
	SqlStats.cpp \
	rts_msg.cpp \
	sql_buffer.cpp \
	stubs.cpp \
	stubs2.cpp \
	SwapSpace.cpp \
	tempfile.cpp \
	timeout_data.cpp \
	TriggerEnable.cpp \
	TupleSpace.cpp \
	UdrExeIpc.cpp \
        ExHdfsScan.cpp \
        JavaObjectInterface.cpp \
        SequenceFileReader.cpp \
        OrcFileReader.cpp \
        HBaseClient_JNI.cpp 
CPPSRC += vers_libexecutor.cpp \
	ssmpipc.cpp \
	CancelBroker.cpp \
	ExCancel.cpp \
	ExSMCommon.cpp \
	ExSMTrace.cpp \
	ExSMGlobals.cpp \
	ExSMTask.cpp \
	ExSMTaskList.cpp \
	ExSMQueue.cpp \
	ExSMReader.cpp \
	ExSMEvent.cpp \
	ExSMExitHandler.cpp \
	ExSMShortMessage.cpp \
	ExSMReadyList.cpp \
        ExFastTransport.cpp 

INCLUDE_DIRS := sqlutils/SQ_tseInterface

ifneq ($(SP_DIS),)
EXTERN_LIBS := $(SP_EXPORT_LIB)/libwrappersq.so 
endif
SYS_LIBS := -lrt -lpthread 
SRCPATH := bin executor runtimestats porting_layer qmscommon

EXTERN_LIBS += $(LIBROOT)/liblog4cpp.so

### Java stuff - for building trafodion-HBaseAccess
################################################

PACKAGE := org/trafodion/sql/HBaseAccess
ORCPACKAGE := org/apache/hadoop/hive/ql/io/orc
JARPREFIX := trafodion-HBaseAccess-$(TRAFODION_VER)

JSRC := RowToInsert.java \
	RowsToInsert.java \
        ResultIterator.java \
        StringArrayList.java \
        ByteArrayList.java \
        ResultKeyValueList.java \
        SequenceFileWriter.java \
        SequenceFileReader.java \
        HTableClient.java \
        HBaseClient.java \
	HiveClient.java \
        HBulkLoadClient.java \
	OrcFileReader.java



# Set up explicit dependencies (necessary for parallel builds... and for correctness).
$(TARGOBJDIR)/java/$(PACKAGE)/HTableClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/RowToInsert.class
$(TARGOBJDIR)/java/$(PACKAGE)/HTableClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/RowsToInsert.class
$(TARGOBJDIR)/java/$(PACKAGE)/HTableClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/ResultIterator.class
$(TARGOBJDIR)/java/$(PACKAGE)/HTableClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/ByteArrayList.class
$(TARGOBJDIR)/java/$(PACKAGE)/HTableClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/ResultKeyValueList.class

$(TARGOBJDIR)/java/$(PACKAGE)/HBulkLoadClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/RowToInsert.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBulkLoadClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/RowsToInsert.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBulkLoadClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/HTableClient.class

$(TARGOBJDIR)/java/$(PACKAGE)/HBaseClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/RowToInsert.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBaseClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/RowsToInsert.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBaseClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/ResultIterator.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBaseClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/StringArrayList.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBaseClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/ByteArrayList.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBaseClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/ResultKeyValueList.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBaseClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/HTableClient.class
$(TARGOBJDIR)/java/$(PACKAGE)/HBaseClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/HBulkLoadClient.class


$(TARGOBJDIR)/java/$(PACKAGE)/HiveClient.class : $(TARGOBJDIR)/java/$(PACKAGE)/StringArrayList.class

  JARMANIFEST := trafodion-HBaseAccess.jar.mf

JSRC_DIR  := $(MY_SQROOT)/../sql/executor
CLASSPATH := $(TARGOBJDIR)/java:$(JSRC_DIR):$(ENV_CLASSPATH)


$(JARMANIFEST) : $(MY_SQROOT)/export/include/SCMBuildMan.mf $(TOPDIR)/executor/trafodion-HBaseAccess.jar.version
	cat  $(TOPDIR)/executor/trafodion-HBaseAccess.jar.version >$@
	cat $(MY_SQROOT)/export/include/SCMBuildMan.mf >>$@

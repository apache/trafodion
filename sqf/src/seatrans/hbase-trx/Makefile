# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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


BLD_HBASE_TRX_JARNAME_1_6   =hbase_jdk1_6-trx-0.94.6.jar
BLD_HBASE_TRX_JARNAME_1_7   =hbase-trx-0.94.6.jar
# Following for Hortonworks.
BLD_HBASE_TRX_JARNAME_1_6_HW=hbase_jdk1_6_hw-trx-0.94.6.jar

TRX_SRC = src/main/java/org/apache/hadoop/hbase/regionserver/transactional/TransactionalRegionServer.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/Dictionary.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/TransactionalRegion.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/CleanOldTransactionsChore.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/THLogKey.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/THLogSplitter.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/THLogRecoveryManager.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/THLog.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/LRUDictionary.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/SingleVersionDeleteNotSupported.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/KeyValueListScanner.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/TransactionState.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/SequenceFileLogWriter.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/CompressionContext.java \
src/main/java/org/apache/hadoop/hbase/regionserver/transactional/TransactionalAggregateImpl.java \
src/main/java/org/apache/hadoop/hbase/regionserver/tableindexed/IndexedRegion.java \
src/main/java/org/apache/hadoop/hbase/regionserver/tableindexed/IndexedRegionServer.java \
src/main/java/org/apache/hadoop/hbase/regionserver/tableindexed/IndexMaintenanceUtils.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/UnknownTransactionException.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/RMInterface.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/JtaXAResource.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/HBaseBackedTransactionLogger.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionalTable.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionRegionLocation.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/RegionHelper.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionLogger.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionState.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/LocalTransactionLogger.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/CommitUnsuccessfulException.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionScannerCallable.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionManager.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionalClientScanner.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionalAggregationClient.java \
src/main/java/org/apache/hadoop/hbase/client/transactional/TransactionalAggregateProtocol.java \
src/main/java/org/apache/hadoop/hbase/client/tableindexed/IndexNotFoundException.java \
src/main/java/org/apache/hadoop/hbase/client/tableindexed/SimpleIndexKeyGenerator.java \
src/main/java/org/apache/hadoop/hbase/client/tableindexed/IndexedTableDescriptor.java \
src/main/java/org/apache/hadoop/hbase/client/tableindexed/IndexedTable.java \
src/main/java/org/apache/hadoop/hbase/client/tableindexed/IndexedTableAdmin.java \
src/main/java/org/apache/hadoop/hbase/client/tableindexed/IndexKeyGenerator.java \
src/main/java/org/apache/hadoop/hbase/client/tableindexed/IndexSpecificationArray.java \
src/main/java/org/apache/hadoop/hbase/client/tableindexed/IndexSpecification.java \
src/main/java/org/apache/hadoop/hbase/ipc/IndexedRegionInterface.java \
src/main/java/org/apache/hadoop/hbase/ipc/TransactionalRegionInterface.java

.NOTPARALLEL: all

all: build_all

$(BLD_HBASE_TRX_JARNAME_1_7): $(TRX_SRC)
	BLD_HBASE_TRX_JAR=$(BLD_HBASE_TRX_JARNAME_1_7) ./build_trx

$(BLD_HBASE_TRX_JARNAME_1_6): $(TRX_SRC)
	JAVA_HOME=$(JAVAJDK16) BLD_HBASE_TRX_JAR=$(BLD_HBASE_TRX_JARNAME_1_6) ./build_trx

# The following target to be built only on the machine with Hortonworks 1.2 installed
$(BLD_HBASE_TRX_JARNAME_1_6_HW): $(TRX_SRC)
	pushd . >/dev/null
	echo $(PWD)
	cd src/main/java/org/apache/hadoop/hbase/regionserver/transactional; echo $(PWD); cp -p THLog.java THLog.java.orig; sed -e "s/\/\/only_for_hortonworks//" < THLog.java.orig > THLog.java
	echo $(PWD)
	JAVA_HOME=$(JAVAJDK16) BLD_HBASE_TRX_JAR=$(BLD_HBASE_TRX_JARNAME_1_6) ./build_trx

jdk_1_7: $(BLD_HBASE_TRX_JARNAME_1_7)

jdk_1_6: $(BLD_HBASE_TRX_JARNAME_1_6)

jdk_1_6_hw: $(BLD_HBASE_TRX_JARNAME_1_6_HW)

build_all: jdk_1_6 jdk_1_7

clean:
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_TRX_JARNAME_1_7)
	$(RM) $(MY_SQROOT)/export/lib/$(BLD_HBASE_TRX_JARNAME_1_6)
	$(RM) $(BLD_HBASE_TRX_JARNAME_1_7)
	$(RM) $(BLD_HBASE_TRX_JARNAME_1_6)
	$(RM) build_trx.log


/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

// SsccTableClient.java

package org.apache.hadoop.hbase.coprocessor.transactional;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NavigableMap;
import java.util.concurrent.atomic.AtomicLong;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.CellUtil;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.KeyValue.Type;
import org.apache.hadoop.hbase.KeyValueUtil;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.TableNotEnabledException;
import org.apache.hadoop.hbase.TableNotFoundException;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.DtmConst;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRegionService;
import org.apache.hadoop.hbase.filter.CompareFilter.CompareOp;
import org.apache.hadoop.hbase.filter.Filter;
import org.apache.hadoop.hbase.filter.SingleColumnValueFilter;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto.MutationType;
import org.apache.hadoop.hbase.regionserver.transactional.IdTm;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmException;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmId;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.Pair;

import com.google.protobuf.ByteString;
import com.google.protobuf.HBaseZeroCopyByteString;

public class SsccTableClientUtils {
    protected final static Log log = LogFactory.getLog(SsccTableClientUtils.class);
    private static final int ID_TM_SERVER_TIMEOUT = 1000;
    static String regionname = "RegionName";
    static HTable ht = null;
    private static long i = 1L;
    private static AtomicLong startId = new AtomicLong(10);
    private static IdTm idServer = new IdTm(false);
    static Map<String, TransactionState> transMap = new HashMap<String, TransactionState>();
    static long scannerId = 0L;
    static int returnStatus = 0;
    static boolean checkResult = false;
    static boolean hasMore = false;
    static long totalRows = 0L;
    static boolean continuePerform = true;
    static byte[][] startKeys = null;
    static int startPos = 0;
    static byte[] startRow = null;
    static byte[] lastRow = null;
    static List<HRegionLocation> regionsList = null;
    static int regionCount = 0;
    static Scan scan = null;
    static Pair<byte[][], byte[][]> startEndKeys = null;

    private static final String TABLE_NAME = "table1";

    private static final byte[] FAMILY = Bytes.toBytes("family");
    private static final byte[] FAMILYBAD = Bytes.toBytes("familybad");
    private static final byte[] QUAL_A = Bytes.toBytes("a");
    private static final byte[] QUAL_B = Bytes.toBytes("b");

    static final String ROW1 = "row1";
    static final String ROW2 = "row2";
    static final String ROW3 = "row3";
    static final String ROW4 = "row4";
    static final String ROW5 = "row5";
    static final String ROW6 = "row6";
    static final int VALUE1 = 1;
    static final int VALUE2 = 2;
    static final int VALUE3 = 3;
    static final int VALUE4 = 4;
    static final int VALUE5 = 5;

    private static final int COMMIT_OK = 1;
    private static final int COMMIT_OK_READ_ONLY = 2;
    private static final int COMMIT_UNSUCCESSFUL = 3;
    private static final int COMMIT_CONFLICT = 5;

    private static final int STATEFUL_UPDATE_OK = 1;
    private static final int STATEFUL_UPDATE_CONFLICT = 2;
    private static final int STATELESS_UPDATE_OK = 3;
    private static final int STATELESS_UPDATE_CONFLICT = 5;

    private static HBaseAdmin admin;

    private static void genTransId() throws IOException {
        // System.out.println("gen  ||  " + Thread.currentThread().getName() +
        // "   ||   " + transMap);

        String threadName = Thread.currentThread().getName();
        TransactionState ts = transMap.get(threadName);
        if (ts == null) {
            ts = new TransactionState(i++);
            IdTmId seqId = new IdTmId();
            try {
                idServer.id(ID_TM_SERVER_TIMEOUT, seqId);
            } catch (IdTmException e) {
                e.printStackTrace();
            }
            ts.setStartId(seqId.val);

            transMap.put(threadName, ts);
        }
    }

    private static void destoryTransId() {
        String threadName = Thread.currentThread().getName();
        transMap.remove(threadName);
    }

    private static long getTransId() {
        // System.out.println("get  ||  " + Thread.currentThread().getName() +
        // "   ||   " + transMap);
        TransactionState ts = transMap.get(Thread.currentThread().getName());
        if (ts == null) {
            return i++;
        } else {
            return ts.getTransactionId();
        }
    }

    private static long getStartId() {
        // System.out.println("get  ||  " + Thread.currentThread().getName() +
        // "   ||   " + transMap);
        TransactionState ts = transMap.get(Thread.currentThread().getName());
        if (ts == null) {
            IdTmId seqId = new IdTmId();
            try {
                idServer.id(ID_TM_SERVER_TIMEOUT, seqId);
            } catch (IdTmException e) {
                e.printStackTrace();
            }
            return seqId.val;
        } else {
            return ts.getStartId();
        }
    }

    // Initialize and set up tables
    public static void initialize() throws Exception {
        // File workaround = new File(".");
        // System.getProperties().put("hadoop.home.dir",
        // workaround.getAbsolutePath());
        // new File("./bin").mkdirs();
        // new File("./bin/winutils.exe").createNewFile();
        Configuration config = HBaseConfiguration.create();
        // config.set("hbase.zookeeper.quorum", "sqws139.houston.hp.com");
        // config.set("hbase.zookeeper.property.clientPort", "48370");
        // config.set("hbase.master", "16.235.163.156:10560");
        // config.set("hbase.zookeeper.quorum", "16.235.163.156");
        HTableDescriptor desc = new HTableDescriptor(TableName.valueOf(TABLE_NAME));
        desc.addFamily(new HColumnDescriptor(FAMILY).setMaxVersions(3));
        desc.addFamily(new HColumnDescriptor(DtmConst.TRANSACTION_META_FAMILY).setMaxVersions(3));
        desc.addCoprocessor("org.apache.hadoop.hbase.coprocessor.transactional.SsccRegionEndpoint");

        admin = new HBaseAdmin(config);

        try {
            log.info("Cleaning up the table " + TABLE_NAME);
            admin.disableTable(TABLE_NAME);
            admin.deleteTable(TABLE_NAME);
        } catch (TableNotFoundException e) {
            log.info("Table " + TABLE_NAME + " was not found");
        } catch (TableNotEnabledException n) {
            log.info("Table " + TABLE_NAME + " is not enabled");
        }

        try {
            log.info("Creating the table " + TABLE_NAME);
            admin.createTable(desc);
        } catch (TableExistsException e) {
            log.info("Table " + TABLE_NAME + " already exists");
        }

        ht = new HTable(config, desc.getName());
        try {
            startKeys = ht.getStartKeys();
            startRow = startKeys[startPos];
            log.info("Table " + TABLE_NAME + " startRow is " + startRow);
        } catch (IOException e) {
            log.info("Table " + TABLE_NAME + " unable to get start keys" + e);
        }
        for (int i = 0; i < startKeys.length; i++) {
            String regionLocation = ht.getRegionLocation(startKeys[i]).getHostname();
            log.info("Table " + TABLE_NAME + " region location " + regionLocation + ", startKey is " + startKeys[i]);
        }

        try {
            startEndKeys = ht.getStartEndKeys();
            for (int i = 0; i < startEndKeys.getFirst().length; i++) {
                log.info("First key: " + startEndKeys.getFirst()[i] + ", Second key: " + startEndKeys.getSecond()[i]);
            }
        } catch (Exception e) {
            log.info("Table " + TABLE_NAME + " unable to get start and endkeys" + e);
        }

        regionsList = ht.getRegionsInRange(HConstants.EMPTY_START_ROW, HConstants.EMPTY_START_ROW);

        int first = 0;
        for (HRegionLocation regionLocation : regionsList) {
            HRegionInfo region = regionLocation.getRegionInfo();
            if (first == 0) {
                regionname = region.getRegionNameAsString();
                first++;
            }

            log.info(region.getRegionNameAsString());
        }
    }

    static public void testSsccAbortTransaction() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccAbortTransaction");
        final long id = getTransId();
        destoryTransId();

        Batch.Call<SsccRegionService, SsccAbortTransactionResponse> callable = new Batch.Call<SsccRegionService, SsccAbortTransactionResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccAbortTransactionResponse> rpcCallback = new BlockingRpcCallback<SsccAbortTransactionResponse>();

            @Override
            public SsccAbortTransactionResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionRequest.Builder builder = SsccAbortTransactionRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                instance.abortTransaction(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccAbortTransactionResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccAbortTransactionResponse aresponse : result.values()) {
            boolean hasException = aresponse.getHasException();
            String exception = aresponse.getException();
            if (hasException) {
                log.info("  " + Thread.currentThread().getName() + "  SsccAbortTransactionResponse exception "
                        + exception);
                throw new IOException(exception);
            }
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccAbortTransaction");
        return;
    }

    static public void testSsccBeginTransaction() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccBeginTransaction");
        genTransId();
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccBeginTransactionResponse> callable = new Batch.Call<SsccRegionService, SsccBeginTransactionResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccBeginTransactionResponse> rpcCallback = new BlockingRpcCallback<SsccBeginTransactionResponse>();

            @Override
            public SsccBeginTransactionResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionRequest.Builder builder = SsccBeginTransactionRequest
                        .newBuilder();

                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                instance.beginTransaction(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        try {
            ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccBeginTransaction with transId: " + id);
        return;
    }

    static public void testSsccCheckAndDelete() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccCheckAndDelete");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse> callable = new Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCheckAndDeleteResponse> rpcCallback = new BlockingRpcCallback<SsccCheckAndDeleteResponse>();

            @Override
            public SsccCheckAndDeleteResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest.Builder builder = SsccCheckAndDeleteRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));
                builder.setRow(HBaseZeroCopyByteString.wrap(Bytes.toBytes(ROW1)));
                builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
                builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
                builder.setValue(HBaseZeroCopyByteString.wrap(Bytes.toBytes(VALUE1)));
                Delete d = new Delete(Bytes.toBytes(ROW1));
                d.deleteColumns(FAMILY, QUAL_A);
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, d);
                builder.setDelete(m1);

                instance.checkAndDelete(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCheckAndDeleteResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccCheckAndDeleteResponse cresponse : result.values()) {
            checkResult = cresponse.getResult();
            String exception = cresponse.getException();
            boolean hasException = cresponse.getHasException();
            if (hasException)
                log.info("  " + Thread.currentThread().getName() + "  testSsccCheckAndDeleteResponse exception "
                        + exception);
            else
                log.info("  " + Thread.currentThread().getName() + "  testSsccCheckAndDeleteResponse result is  "
                        + checkResult);
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccCheckAndDelete");
        return;
    }

    static public void testSsccCheckAndDelete2() throws IOException {

        log.info("Starting testSsccCheckAndDelete2");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse> callable = new Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCheckAndDeleteResponse> rpcCallback = new BlockingRpcCallback<SsccCheckAndDeleteResponse>();

            @Override
            public SsccCheckAndDeleteResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest.Builder builder = SsccCheckAndDeleteRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));
                builder.setRow(HBaseZeroCopyByteString.wrap(Bytes.toBytes(ROW1)));
                builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
                builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_B));
                builder.setValue(HBaseZeroCopyByteString.wrap(Bytes.toBytes(VALUE2)));
                Delete d = new Delete(Bytes.toBytes(ROW1));
                d.deleteColumns(FAMILY, QUAL_B);
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, d);
                builder.setDelete(m1);

                instance.checkAndDelete(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCheckAndDeleteResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccCheckAndDeleteResponse cresponse : result.values()) {
            checkResult = cresponse.getResult();
            String exception = cresponse.getException();
            boolean hasException = cresponse.getHasException();
            if (hasException)
                log.info("  testSsccCheckAndDelete2Response exception " + exception);
            else
                log.info("  testSsccCheckAndDelete2Response result is  " + checkResult);
        }

        log.info("Finished testSsccCheckAndDelete2");
        return;
    }

    static public void testSsccCheckAndDelete4() throws IOException {

        log.info("Starting testSsccCheckAndDelete");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse> callable = new Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCheckAndDeleteResponse> rpcCallback = new BlockingRpcCallback<SsccCheckAndDeleteResponse>();

            @Override
            public SsccCheckAndDeleteResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest.Builder builder = SsccCheckAndDeleteRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));
                builder.setRow(HBaseZeroCopyByteString.wrap(Bytes.toBytes(ROW2)));
                builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
                builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
                builder.setValue(HBaseZeroCopyByteString.wrap(Bytes.toBytes(VALUE2)));
                Delete d = new Delete(Bytes.toBytes(ROW2));
                d.deleteColumns(FAMILY, QUAL_A);
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, d);
                builder.setDelete(m1);

                instance.checkAndDelete(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCheckAndDeleteResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccCheckAndDeleteResponse cresponse : result.values()) {
            checkResult = cresponse.getResult();
            String exception = cresponse.getException();
            boolean hasException = cresponse.getHasException();
            if (hasException)
                log.info("  testSsccCheckAndDeleteResponse exception " + exception);
            else
                log.info("  testSsccCheckAndDeleteResponse result is  " + checkResult);
        }

        log.info("Finished testSsccCheckAndDelete");
        return;
    }

    static public void testSsccCheckAndPut() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccCheckAndPut");
        final byte[] emptyVal = new byte[] {};
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable = new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback = new BlockingRpcCallback<SsccCheckAndPutResponse>();

            @Override
            public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));
                builder.setRow(HBaseZeroCopyByteString.wrap(Bytes.toBytes(ROW1)));
                builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
                builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
                builder.setValue(HBaseZeroCopyByteString.wrap(emptyVal));
                Put p = new Put(Bytes.toBytes(ROW1)).add(FAMILY, QUAL_A, Bytes.toBytes(1));
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
                builder.setPut(m1);

                instance.checkAndPut(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCheckAndPutResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        for (SsccCheckAndPutResponse cresponse : result.values()) {
            checkResult = cresponse.getResult();
            String exception = cresponse.getException();
            boolean hasException = cresponse.getHasException();
            if (hasException)
                log.info("  " + Thread.currentThread().getName() + "  testSsccCheckAndPutResponse exception "
                        + exception);
            else
                log.info("  " + Thread.currentThread().getName() + "  testSsccCheckAndPutResponse result is  "
                        + checkResult);
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccCheckAndPut");
        return;
    }

    static public void testSsccCheckAndPut2() throws IOException {

        log.info("Starting testSsccCheckAndPut2");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable = new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback = new BlockingRpcCallback<SsccCheckAndPutResponse>();

            @Override
            public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));
                builder.setRow(HBaseZeroCopyByteString.wrap(Bytes.toBytes(ROW1)));
                builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
                builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
                builder.setValue(HBaseZeroCopyByteString.wrap(Bytes.toBytes(VALUE1)));
                Put p = new Put(Bytes.toBytes(ROW1)).add(FAMILY, QUAL_B, Bytes.toBytes(2));
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
                builder.setPut(m1);

                instance.checkAndPut(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCheckAndPutResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        for (SsccCheckAndPutResponse cresponse : result.values()) {
            checkResult = cresponse.getResult();
            String exception = cresponse.getException();
            boolean hasException = cresponse.getHasException();
            if (hasException)
                log.info("  testSsccCheckAndPut2Response exception " + exception);
            else
                log.info("  testSsccCheckAndPut2Response result is  " + checkResult);
        }

        log.info("Finished testSsccCheckAndPut2");
        return;
    }

    static public void testSsccCheckAndPut3() throws IOException {

        log.info("Starting testSsccCheckAndPut3");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable = new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback = new BlockingRpcCallback<SsccCheckAndPutResponse>();

            @Override
            public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));
                builder.setRow(HBaseZeroCopyByteString.wrap(Bytes.toBytes(ROW1)));
                builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
                builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
                builder.setValue(HBaseZeroCopyByteString.wrap(Bytes.toBytes(VALUE1)));
                Put p = new Put(Bytes.toBytes(ROW2)).add(FAMILY, QUAL_A, Bytes.toBytes(1));
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
                builder.setPut(m1);

                instance.checkAndPut(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCheckAndPutResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        for (SsccCheckAndPutResponse cresponse : result.values()) {
            checkResult = cresponse.getResult();
            String exception = cresponse.getException();
            boolean hasException = cresponse.getHasException();
            if (hasException)
                log.info("  testSsccCheckAndPut3Response exception " + exception);
            else
                log.info("  testSsccCheckAndPut3Response result is  " + checkResult);
        }

        log.info("Finished testSsccCheckAndPut3");
        return;
    }

    static public void testSsccCheckAndPut4() throws IOException {

        log.info("Starting testSsccCheckAndPut4");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable = new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback = new BlockingRpcCallback<SsccCheckAndPutResponse>();

            @Override
            public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));
                builder.setRow(HBaseZeroCopyByteString.wrap(Bytes.toBytes(ROW2)));
                builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
                builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
                builder.setValue(HBaseZeroCopyByteString.wrap(Bytes.toBytes(VALUE2)));
                Put p = new Put(Bytes.toBytes(ROW2)).add(FAMILY, QUAL_A, Bytes.toBytes(2));
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
                builder.setPut(m1);

                instance.checkAndPut(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCheckAndPutResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        for (SsccCheckAndPutResponse cresponse : result.values()) {
            checkResult = cresponse.getResult();
            String exception = cresponse.getException();
            boolean hasException = cresponse.getHasException();
            if (hasException)
                log.info("  testSsccCheckAndPut4Response exception " + exception);
            else
                log.info("  testSsccCheckAndPut4Response result is  " + checkResult);
        }

        log.info("Finished testSsccCheckAndPut4");
        return;
    }

    static public void testSsccCloseScanner() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccClosecanner");
        final long id = getTransId();
        Batch.Call<SsccRegionService, SsccCloseScannerResponse> callable = new Batch.Call<SsccRegionService, SsccCloseScannerResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCloseScannerResponse> rpcCallback = new BlockingRpcCallback<SsccCloseScannerResponse>();

            @Override
            public SsccCloseScannerResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerRequest.Builder builder = SsccCloseScannerRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setScannerId(scannerId++);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                instance.closeScanner(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCloseScannerResponse> result = null;

        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccCloseScannerResponse cresponse : result.values()) {
            boolean hasException = cresponse.getHasException();
            String exception = cresponse.getException();
            if (hasException)
                log.info("  " + Thread.currentThread().getName() + "  testSsccCloseScannerResponse exception "
                        + exception);
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccCloseScanner");
        return;
    }

    static public void testSsccCommit() throws IOException {

        final long id = getTransId();
        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccCommit with transId: " + id);
        destoryTransId();
        Batch.Call<SsccRegionService, SsccCommitResponse> callable = new Batch.Call<SsccRegionService, SsccCommitResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCommitResponse> rpcCallback = new BlockingRpcCallback<SsccCommitResponse>();

            @Override
            public SsccCommitResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequest.Builder builder = SsccCommitRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                instance.commit(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCommitResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccCommitResponse cresponse : result.values()) {
            String exception = cresponse.getException();
            boolean hasException = cresponse.getHasException();
            if (hasException) {
                log.info("  " + Thread.currentThread().getName() + "  SsccCommitResponse exception " + exception);
                throw new IOException(exception);
            }
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccCommit");
        return;
    }

    static public void testSsccCommitRequest() throws IOException {
        final long id = getTransId();
        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccCommitRequest with transId: " + id);

        Batch.Call<SsccRegionService, SsccCommitRequestResponse> callable = new Batch.Call<SsccRegionService, SsccCommitRequestResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCommitRequestResponse> rpcCallback = new BlockingRpcCallback<SsccCommitRequestResponse>();

            @Override
            public SsccCommitRequestResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestRequest.Builder builder = SsccCommitRequestRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                instance.commitRequest(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCommitRequestResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccCommitRequestResponse cresponse : result.values()) {
            int value = cresponse.getResult();
            String returnString;
            switch (value) {
            case COMMIT_OK:
                returnString = new String("COMMIT_OK");
                break;
            case COMMIT_OK_READ_ONLY:
                returnString = new String("COMMIT_OK_READ_ONLY");
                break;
            case COMMIT_UNSUCCESSFUL:
                returnString = new String("COMMIT_UNSUCCESSFUL");
                break;
            case COMMIT_CONFLICT:
                returnString = new String("COMMIT_CONFLICT");
                break;
            default:
                returnString = new String("Unknown return value: " + Integer.toString(value));
                break;
            }
            log.info("  " + Thread.currentThread().getName() + "  SsccCommitRequestResponse value " + returnString);
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccCommitRequest");
        return;
    }

    static public void testSsccCommitIfPossible() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccCommitIfPossible");
        final long id = getTransId();
        destoryTransId();
        Batch.Call<SsccRegionService, SsccCommitIfPossibleResponse> callable = new Batch.Call<SsccRegionService, SsccCommitIfPossibleResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccCommitIfPossibleResponse> rpcCallback = new BlockingRpcCallback<SsccCommitIfPossibleResponse>();

            @Override
            public SsccCommitIfPossibleResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleRequest.Builder builder = SsccCommitIfPossibleRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                instance.commitIfPossible(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccCommitIfPossibleResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccCommitIfPossibleResponse cipresponse : result.values()) {
            boolean hasException = cipresponse.getHasException();
            if (hasException) {
                String exception = cipresponse.getException();
                log.info("  " + Thread.currentThread().getName() + "  testSsccCommitIfPossible exception " + exception);
            } else {
                log.info("  " + Thread.currentThread().getName() + "  testSsccCommitIfPossible result is "
                        + cipresponse.getWasSuccessful());
            }

        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccCommitIfPossible");
        return;
    }

    static public void testSsccDelete(final int value) throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccDelete");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccDeleteTransactionalResponse> callable = new Batch.Call<SsccRegionService, SsccDeleteTransactionalResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccDeleteTransactionalResponse> rpcCallback = new BlockingRpcCallback<SsccDeleteTransactionalResponse>();

            @Override
            public SsccDeleteTransactionalResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalRequest.Builder builder = SsccDeleteTransactionalRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                Delete d = new Delete(Bytes.toBytes(ROW1));

                // d.deleteColumn(FAMILY, QUAL_A, System.currentTimeMillis());
                NavigableMap<byte[], List<Cell>> map = d.getFamilyCellMap();
                List<Cell> list = new ArrayList<Cell>();
                Cell c = new KeyValue(Bytes.toBytes(ROW1), FAMILY, QUAL_A, System.currentTimeMillis(), Type.Delete);

                list.add(KeyValueUtil.ensureKeyValue(c));
                map.put(FAMILY, list);
                d.setFamilyCellMap(map);

                MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, d);
                builder.setDelete(m1);

                instance.delete(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccDeleteTransactionalResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccDeleteTransactionalResponse dresponse : result.values()) {
            boolean hasException = dresponse.getHasException();
            if (hasException) {
                String exception = dresponse.getException();
                log.info("  " + Thread.currentThread().getName() + "  testSsccDelete exception " + exception);
            } else {
                returnStatus = dresponse.getStatus();
                String returnString;

                switch (returnStatus) {
                case STATEFUL_UPDATE_OK:
                    returnString = new String("STATEFUL_UPDATE_OK");
                    break;
                case STATEFUL_UPDATE_CONFLICT:
                    returnString = new String("STATEFUL_UPDATE_CONFLICT");
                    break;
                case STATELESS_UPDATE_OK:
                    returnString = new String("STATELESS_UPDATE_OK");
                    break;
                case STATELESS_UPDATE_CONFLICT:
                    returnString = new String("STATELESS_UPDATE_CONFLICT");
                    break;
                default:
                    returnString = new String("Unknown return value: " + Integer.toString(returnStatus));
                    break;

                }
                log.info("  " + Thread.currentThread().getName() + "  testSsccDelete returnStatus is  " + returnString);
            }

        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccDelete");
        return;
    }

    static public void testSsccDeleteMultiple() throws IOException {

        log.info("Starting testSsccDeleteMultiple");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccDeleteMultipleTransactionalResponse> callable = new Batch.Call<SsccRegionService, SsccDeleteMultipleTransactionalResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccDeleteMultipleTransactionalResponse> rpcCallback = new BlockingRpcCallback<SsccDeleteMultipleTransactionalResponse>();

            @Override
            public SsccDeleteMultipleTransactionalResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalRequest.Builder builder = SsccDeleteMultipleTransactionalRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                Delete d1 = new Delete(Bytes.toBytes(ROW1));
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, d1);
                builder.addDelete(m1);
                Delete d2 = new Delete(Bytes.toBytes(ROW2));
                MutationProto m2 = ProtobufUtil.toMutation(MutationType.DELETE, d2);
                builder.addDelete(m2);
                Delete d3 = new Delete(Bytes.toBytes(ROW3));
                MutationProto m3 = ProtobufUtil.toMutation(MutationType.DELETE, d3);
                builder.addDelete(m3);
                Delete d4 = new Delete(Bytes.toBytes(ROW4));
                MutationProto m4 = ProtobufUtil.toMutation(MutationType.DELETE, d4);
                builder.addDelete(m4);
                Delete d5 = new Delete(Bytes.toBytes(ROW5));
                MutationProto m5 = ProtobufUtil.toMutation(MutationType.DELETE, d5);
                builder.addDelete(m5);
                Delete d6 = new Delete(Bytes.toBytes(ROW6));
                MutationProto m6 = ProtobufUtil.toMutation(MutationType.DELETE, d6);
                builder.addDelete(m6);

                instance.deleteMultiple(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccDeleteMultipleTransactionalResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccDeleteMultipleTransactionalResponse dmresponse : result.values()) {
            boolean hasException = dmresponse.getHasException();
            if (hasException) {
                String exception = dmresponse.getException();
                log.info("  testSsccDeleteMultiple exception " + exception);
            } else {
                returnStatus = dmresponse.getStatus();
                String returnString;

                switch (returnStatus) {
                case STATEFUL_UPDATE_OK:
                    returnString = new String("STATEFUL_UPDATE_OK");
                    break;
                case STATEFUL_UPDATE_CONFLICT:
                    returnString = new String("STATEFUL_UPDATE_CONFLICT");
                    break;
                case STATELESS_UPDATE_OK:
                    returnString = new String("STATELESS_UPDATE_OK");
                    break;
                case STATELESS_UPDATE_CONFLICT:
                    returnString = new String("STATELESS_UPDATE_CONFLICT");
                    break;
                default:
                    returnString = new String("Unknown return value: " + Integer.toString(returnStatus));
                    break;

                }
                log.info("  testSsccDeleteMultiple returnStatus is  " + returnString);
            }

        }

        log.info("Finished testSsccDeleteMultiple");
        return;
    }

    public static Integer[] testSsccGet() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccGet");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccGetTransactionalResponse> callable = new Batch.Call<SsccRegionService, SsccGetTransactionalResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccGetTransactionalResponse> rpcCallback = new BlockingRpcCallback<SsccGetTransactionalResponse>();

            @Override
            public SsccGetTransactionalResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalRequest.Builder builder = SsccGetTransactionalRequest
                        .newBuilder();
                // Get get = new Get(ROW1).addColumn(FAMILY, Bytes.toBytes(1));
                Get get = new Get(Bytes.toBytes(ROW1)).addColumn(FAMILY, QUAL_A);
                builder.setGet(ProtobufUtil.toGet(get));
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                instance.get(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccGetTransactionalResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        List<Integer> results = new ArrayList<Integer>();
        for (SsccGetTransactionalResponse gresponse : result.values()) {
            Result resultFromGet = ProtobufUtil.toResult(gresponse.getResult());
            log.info("  " + Thread.currentThread().getName()
                    + "  SsccGetTransactionalResponse Get result count before action is committed: "
                    + resultFromGet.size());
            if (resultFromGet.size() == 0) {
                log.info("  " + Thread.currentThread().getName() + "  can't get any value. ");
                continue;
            }

            for (Cell c : resultFromGet.listCells()) {
                log.info("  " + Thread.currentThread().getName() + "  get value is : "
                        + Bytes.toInt(CellUtil.cloneValue(c)));
                results.add(Bytes.toInt(CellUtil.cloneValue(c)));
            }
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccGet");
        return results.toArray(new Integer[results.size()]);
    }

    static public void testSsccPut(final int value) throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccPut, value is :" + value);
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable = new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback = new BlockingRpcCallback<SsccPutTransactionalResponse>();

            @Override
            public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                Put p = new Put(Bytes.toBytes(ROW1)).add(FAMILY, QUAL_A, Bytes.toBytes(value));
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
                builder.setPut(m1);
                instance.put(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccPutTransactionalResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccPutTransactionalResponse presponse : result.values()) {
            boolean hasException = presponse.getHasException();
            if (hasException) {
                String exception = presponse.getException();
                log.info("  " + Thread.currentThread().getName() + "  testSsccPut, value is :" + value + "; exception "
                        + exception);
            } else {
                returnStatus = presponse.getStatus();
                String returnString;

                switch (returnStatus) {
                case STATEFUL_UPDATE_OK:
                    returnString = new String("STATEFUL_UPDATE_OK");
                    break;
                case STATEFUL_UPDATE_CONFLICT:
                    returnString = new String("STATEFUL_UPDATE_CONFLICT");
                    break;
                case STATELESS_UPDATE_OK:
                    returnString = new String("STATELESS_UPDATE_OK");
                    break;
                case STATELESS_UPDATE_CONFLICT:
                    returnString = new String("STATELESS_UPDATE_CONFLICT");
                    break;
                default:
                    returnString = new String("Unknown return value: " + Integer.toString(returnStatus));
                    break;

                }
                log.info("  " + Thread.currentThread().getName() + "  testSsccPut, value is :" + value
                        + "; returnStatus is  " + returnString);
            }
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccPut, value is :" + value);
        return;
    }

    static public void testSsccPutRow(final String row) throws IOException {
        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccPutRow, row is " + row);
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable = new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {

            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccPutTransactionalResponse> done = new BlockingRpcCallback<SsccPutTransactionalResponse>();

            @Override
            public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
                SsccPutTransactionalRequest.Builder request = SsccPutTransactionalRequest.newBuilder();
                request.setTransactionId(id);
                request.setStartId(start);
                request.setRegionName(ByteString.copyFromUtf8(regionname));

                Put p = new Put(Bytes.toBytes(row)).add(FAMILY, QUAL_A, Bytes.toBytes(VALUE1));
                MutationProto m = ProtobufUtil.toMutation(MutationType.PUT, p);
                request.setPut(m);

                instance.put(controller, request.build(), done);
                return done.get();
            }
        };

        try {
            ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccPutRow, row is " + new String(row));
    }

    static public void testSsccPutException() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccPutException");
        final long id = getTransId();
        Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable = new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback = new BlockingRpcCallback<SsccPutTransactionalResponse>();

            @Override
            public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                Put p = new Put(Bytes.toBytes(ROW1)).add(FAMILYBAD, QUAL_A, Bytes.toBytes(1));
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
                builder.setPut(m1);

                instance.put(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        try {
            ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccPutException");
        return;
    }

    static public void testSsccPutMultiple() throws IOException {

        log.info("Starting testSsccPutMultiple");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccPutMultipleTransactionalResponse> callable = new Batch.Call<SsccRegionService, SsccPutMultipleTransactionalResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccPutMultipleTransactionalResponse> rpcCallback = new BlockingRpcCallback<SsccPutMultipleTransactionalResponse>();

            @Override
            public SsccPutMultipleTransactionalResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalRequest.Builder builder = SsccPutMultipleTransactionalRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                Put p = new Put(Bytes.toBytes(ROW1)).add(FAMILY, QUAL_A, Bytes.toBytes(1));
                MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
                builder.addPut(m1);

                instance.putMultiple(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccPutMultipleTransactionalResponse> result = null;
        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccPutMultipleTransactionalResponse pmresponse : result.values()) {
            boolean hasException = pmresponse.getHasException();
            if (hasException) {
                String exception = pmresponse.getException();
                log.info("  testSsccPutMultiple exception " + exception);
            } else {
                returnStatus = pmresponse.getStatus();
                String returnString;

                switch (returnStatus) {
                case STATEFUL_UPDATE_OK:
                    returnString = new String("STATEFUL_UPDATE_OK");
                    break;
                case STATEFUL_UPDATE_CONFLICT:
                    returnString = new String("STATEFUL_UPDATE_CONFLICT");
                    break;
                case STATELESS_UPDATE_OK:
                    returnString = new String("STATELESS_UPDATE_OK");
                    break;
                case STATELESS_UPDATE_CONFLICT:
                    returnString = new String("STATELESS_UPDATE_CONFLICT");
                    break;
                default:
                    returnString = new String("Unknown return value: " + Integer.toString(returnStatus));
                    break;

                }
                log.info("  testSsccPutMultiple returnStatus is  " + returnString);
            }

        }

        log.info("Finished testSsccPutMultiple");
        return;
    }

    static public List<Object[]> testSsccPerformScan() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccPerformScan");
        final long id = getTransId();
        final long start = getStartId();

        Batch.Call<SsccRegionService, SsccPerformScanResponse> callable = new Batch.Call<SsccRegionService, SsccPerformScanResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccPerformScanResponse> rpcCallback = new BlockingRpcCallback<SsccPerformScanResponse>();

            @Override
            public SsccPerformScanResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanRequest.Builder builder = SsccPerformScanRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));
                builder.setScannerId(scannerId);
                builder.setNumberOfRows(9);
                builder.setCloseScanner(false);
                builder.setNextCallSeq(0);

                instance.performScan(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccPerformScanResponse> presult = null;

        try {
            presult = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            log.info("  " + Thread.currentThread().getName()
                    + "  testSsccPerformScanResponse exception getting results " + e);
        }

        int count = 0;
        boolean hasMore = false;

        List<Object[]> results = new ArrayList<Object[]>();

        for (SsccPerformScanResponse presponse : presult.values()) {
            if (presponse.getHasException()) {
                String exception = presponse.getException();
                log.info("  " + Thread.currentThread().getName() + "  testSsccPerformScanResponse exception "
                        + exception);
            } else {
                count = presponse.getResultCount();
                hasMore = presponse.getHasMore();
                log.info("  " + Thread.currentThread().getName() + "  testSsccPerformScan response count " + count
                        + " rows  , hasMore is " + hasMore);
                for (int i = 0; i < count; i++) {
                    Result resultFromScan = ProtobufUtil.toResult(presponse.getResult(i));
                    List<Cell> cells = resultFromScan.listCells();
                    for (Cell c : cells) {
                        Object[] objArrs = { CellUtil.cloneRow(c), CellUtil.cloneFamily(c), CellUtil.cloneQualifier(c),
                                CellUtil.cloneValue(c), c.getTimestamp() };
                        results.add(objArrs);
                    }
                    log.info("  " + Thread.currentThread().getName() + " , result " + resultFromScan);
                }
            }
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccPerformScan");
        return results;
    }

    static public void testSsccOpenScanner() throws IOException {

        log.info("  " + Thread.currentThread().getName() + "  Starting testSsccOpenScanner");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccOpenScannerResponse> callable = new Batch.Call<SsccRegionService, SsccOpenScannerResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccOpenScannerResponse> rpcCallback = new BlockingRpcCallback<SsccOpenScannerResponse>();

            @Override
            public SsccOpenScannerResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerRequest.Builder builder = SsccOpenScannerRequest
                        .newBuilder();
                builder.setTransactionId(id);
                builder.setStartId(start);
                builder.setRegionName(ByteString.copyFromUtf8(regionname));

                Scan scan = new Scan();
                Filter filter = new SingleColumnValueFilter(FAMILY, QUAL_A, CompareOp.GREATER_OR_EQUAL,
                        Bytes.toBytes(VALUE1));
                scan.setFilter(filter);
                scan.addFamily(FAMILY);
                scan.addColumn(FAMILY, QUAL_A);
                builder.setScan(ProtobufUtil.toScan(scan));

                instance.openScanner(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccOpenScannerResponse> result = null;

        try {
            result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        for (SsccOpenScannerResponse oresponse : result.values()) {
            scannerId = oresponse.getScannerId();
            String exception = oresponse.getException();
            boolean hasException = oresponse.getHasException();
            if (hasException)
                log.info("  " + Thread.currentThread().getName() + "  testSsccOpenScannerResponse exception "
                        + exception);
            else
                log.info("  " + Thread.currentThread().getName() + "  testSsccOpenScannerResponse scannerId is "
                        + scannerId);
        }

        log.info("  " + Thread.currentThread().getName() + "  Finished testSsccOpenScanner");
        return;
    }

    static public void testSsccRecoveryRequest() throws IOException {

        log.info("Starting testSsccRecoveryRequest");
        final long id = getTransId();
        final long start = getStartId();
        Batch.Call<SsccRegionService, SsccRecoveryRequestResponse> callable = new Batch.Call<SsccRegionService, SsccRecoveryRequestResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccRecoveryRequestResponse> rpcCallback = new BlockingRpcCallback<SsccRecoveryRequestResponse>();

            @Override
            public SsccRecoveryRequestResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestRequest.Builder rbuilder = SsccRecoveryRequestRequest
                        .newBuilder();
                rbuilder.setTransactionId(id);
                rbuilder.setStartId(start);
                rbuilder.setRegionName(ByteString.copyFromUtf8(regionname));
                rbuilder.setTmId(7);

                instance.recoveryRequest(controller, rbuilder.build(), rpcCallback);
                return rpcCallback.get();
            }
        };

        Map<byte[], SsccRecoveryRequestResponse> rresult = null;
        try {
            rresult = ht.coprocessorService(SsccRegionService.class, null, null, callable);
        } catch (Throwable e) {
            e.printStackTrace();
        }

        int count = 0;
        long l = 0;

        for (SsccRecoveryRequestResponse rresponse : rresult.values()) {
            count = rresponse.getResultCount();
            l = rresponse.getResult(0);
            log.info("  testSsccRecoveryResponse count " + count + ", result " + l);
        }

        log.info("Finished testSsccRecoveryRequest");
        return;
    }

}

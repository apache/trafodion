// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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

package org.apache.hadoop.hbase.coprocessor.transactional;

import java.io.IOException;
import java.util.List;
import java.util.Map;

import org.apache.hadoop.hbase.client.DtmConst;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.TableNotFoundException;
import org.apache.hadoop.hbase.TableNotEnabledException;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto.MutationType;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import com.google.protobuf.HBaseZeroCopyByteString;
import com.google.protobuf.ByteString;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.*;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.Pair;

import org.apache.hadoop.hbase.regionserver.transactional.IdTm;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmException;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmId;

public class SsccTableClient {

  static String regionname = "RegionName";
  static HTable ht = null;
  static long transId = 0L;
  static long startId = 0L;
  static long commitId = 0L;
  static long scannerId = 0L;
  static int returnStatus = 0;
  static boolean checkResult = false;
  static boolean hasMore = false;
  static long totalRows = 0L;
  static boolean continuePerform = true;
  static byte [][] startKeys = null;
  static int startPos = 0;
  static byte [] startRow = null;
  static byte [] lastRow = null;
  private static Result lastResult = null;
  static List<HRegionLocation> regionsList = null;
  static int regionCount = 0;
  static Scan scan = null;
  private HRegionInfo currentRegion = null;
  static Pair<byte[][], byte[][]> startEndKeys = null;

  private static IdTm idServer;
  private static IdTmId tmId;
  private static final int ID_TM_SERVER_TIMEOUT = 1000;

  private static final String TABLE_NAME = "table1";

  private static final byte[] FAMILY = Bytes.toBytes("family");
  private static final byte[] FAMILYBAD = Bytes.toBytes("familybad");
  private static final byte[] QUAL_A = Bytes.toBytes("a");
  private static final byte[] QUAL_B = Bytes.toBytes("b");

  private static final byte[] ROW1 = Bytes.toBytes("row1");
  private static final byte[] ROW2 = Bytes.toBytes("row2");
  private static final byte[] ROW3 = Bytes.toBytes("row3");
  private static final byte[] ROW4 = Bytes.toBytes("row4");
  private static final byte[] ROW5 = Bytes.toBytes("row5");
  private static final byte[] ROW6 = Bytes.toBytes("row6");
  private static final byte [] VALUE = Bytes.toBytes("testValue");
  private static final byte [] VALUE1 = Bytes.toBytes(1);
  private static final byte [] VALUE2 = Bytes.toBytes(2);

  private static final int COMMIT_OK = 1;
  private static final int COMMIT_OK_READ_ONLY = 2;
  private static final int COMMIT_UNSUCCESSFUL = 3;
  private static final int COMMIT_CONFLICT = 5;

  private static final int STATEFUL_UPDATE_OK = 1;
  private static final int STATEFUL_UPDATE_CONFLICT = 2;
  private static final int STATELESS_UPDATE_OK = 3;
  private static final int STATELESS_UPDATE_CONFLICT = 5;

  private static HBaseAdmin admin;

 // Initialize and set up tables 
    public static void initialize() throws Exception {

     try {
        idServer = new IdTm(false);
     }
     catch (Exception e){
        System.out.println("Exception creating new IdTm: " + e);
     }
     tmId = new IdTmId();
     try {
        bumpTransIds();
     }
     catch (Exception e){
        System.out.println("Exception bumpTransIds " + e);
     }


     Configuration config = HBaseConfiguration.create();

     HTableDescriptor desc = new HTableDescriptor(TABLE_NAME);
     desc.addFamily(new HColumnDescriptor(FAMILY));
     desc.addFamily(new HColumnDescriptor(DtmConst.TRANSACTION_META_FAMILY));

     admin = new HBaseAdmin(config);

     try {
       System.out.println ("  Cleaning up the table " + TABLE_NAME);
       admin.disableTable(TABLE_NAME);
       admin.deleteTable(TABLE_NAME);
     }
     catch (TableNotFoundException e) {
       System.out.println("  Table " + TABLE_NAME + " was not found");
     }
     catch (TableNotEnabledException n) {
       System.out.println("  Table " + TABLE_NAME + " is not enabled");
     }

     try {
       System.out.println ("  Creating the table " + TABLE_NAME);
       admin.createTable(desc);
     }
     catch (TableExistsException e) {
       System.out.println("  Table " + TABLE_NAME + " already exists");
     }

     ht = new HTable(config, desc.getName());
     try {
       startKeys = ht.getStartKeys();
       startRow = startKeys[startPos];
       System.out.println("  Table " + TABLE_NAME + " startRow is " + startRow);
     } catch (IOException e) {
       System.out.println("  Table " + TABLE_NAME + " unable to get start keys" + e);
       System.exit(1);
     }
     for (int i = 0; i < startKeys.length; i++){
       String regionLocation = ht.getRegionLocation(startKeys[i]).getHostname();
       System.out.println("  Table " + TABLE_NAME + " region location" + regionLocation + ", startKey is " + startKeys[i]);
     }

     try {
        startEndKeys = ht.getStartEndKeys();
        for (int i = 0; i < startEndKeys.getFirst().length; i++) {
          System.out.println(" First key: " + startEndKeys.getFirst()[i] +  ", Second key: "  + startEndKeys.getSecond()[i]);
        }
     } catch (Exception e) {
       System.out.println("  Table " + TABLE_NAME + " unable to get start and endkeys" + e);
     }


     regionsList = ht.getRegionsInRange(HConstants.EMPTY_START_ROW, HConstants.EMPTY_START_ROW);

     int first = 0;
     for (HRegionLocation regionLocation : regionsList) {
        HRegionInfo region = regionLocation.getRegionInfo();
        if (first == 0) {
          regionname = region.getRegionNameAsString();
          first++;
        }

        System.out.println("\t\t" + region.getRegionNameAsString());
     }
  }

  static public void testSsccAbortTransaction() throws IOException {

    System.out.println("Starting testSsccAbortTransaction");

    Batch.Call<SsccRegionService, SsccAbortTransactionResponse> callable =
        new Batch.Call<SsccRegionService, SsccAbortTransactionResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccAbortTransactionResponse> rpcCallback =
        new BlockingRpcCallback<SsccAbortTransactionResponse>();

      @Override
      public SsccAbortTransactionResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionRequest.Builder builder = SsccAbortTransactionRequest.newBuilder();
        builder.setTransactionId(transId);
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

      for (SsccAbortTransactionResponse aresponse : result.values())
      {
        boolean hasException = aresponse.getHasException();
        String exception = aresponse.getException();
        if (hasException)
        {
          System.out.println("SsccAbortTransactionResponse exception " + exception );
          throw new IOException(exception);
        }
      }

    System.out.println("Finished testSsccAbortTransaction");
    return;
  }

  static public void testSsccBeginTransaction() throws IOException {

    System.out.println("Starting testSsccBeginTransaction");

    Batch.Call<SsccRegionService, SsccBeginTransactionResponse> callable =
        new Batch.Call<SsccRegionService, SsccBeginTransactionResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccBeginTransactionResponse> rpcCallback =
        new BlockingRpcCallback<SsccBeginTransactionResponse>();

      @Override
      public SsccBeginTransactionResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionRequest.Builder builder = SsccBeginTransactionRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        instance.beginTransaction(controller, builder.build(), rpcCallback);
        return rpcCallback.get();
      }
    };

      Map<byte[], SsccBeginTransactionResponse> result = null;
      try {
        result = ht.coprocessorService(SsccRegionService.class, null, null, callable);
      } catch (Throwable e) {
        e.printStackTrace();
      }

    System.out.println("Finished testSsccBeginTransaction with transId: " + transId);
    return;
  }

  static public void testSsccCheckAndDelete() throws IOException {

    System.out.println("Starting testSsccCheckAndDelete");
    final byte[] emptyVal = new byte[] {};

    Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse> callable =
        new Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndDeleteResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndDeleteResponse>();

      @Override
      public SsccCheckAndDeleteResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest.Builder builder = SsccCheckAndDeleteRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setRow(HBaseZeroCopyByteString.wrap(ROW1));
        builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
        builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
        builder.setValue(HBaseZeroCopyByteString.wrap(VALUE1));
        Delete d = new Delete(ROW1);
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

      for (SsccCheckAndDeleteResponse cresponse : result.values())
      {
        checkResult = cresponse.getResult();
        String exception = cresponse.getException();
        boolean hasException = cresponse.getHasException();
        if (hasException)
          System.out.println("  testSsccCheckAndDeleteResponse exception " + exception );
        else
          System.out.println("  testSsccCheckAndDeleteResponse result is  " + checkResult);
      }

    System.out.println("Finished testSsccCheckAndDelete");
    return;
  }

  static public void testSsccCheckAndDelete2() throws IOException {

    System.out.println("Starting testSsccCheckAndDelete2");
    final byte[] emptyVal = new byte[] {};

    Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse> callable =
        new Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndDeleteResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndDeleteResponse>();

      @Override
      public SsccCheckAndDeleteResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest.Builder builder = SsccCheckAndDeleteRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setRow(HBaseZeroCopyByteString.wrap(ROW1));
        builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
        builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_B));
        builder.setValue(HBaseZeroCopyByteString.wrap(VALUE2));
        Delete d = new Delete(ROW1);
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

      for (SsccCheckAndDeleteResponse cresponse : result.values())
      {
        checkResult = cresponse.getResult();
        String exception = cresponse.getException();
        boolean hasException = cresponse.getHasException();
        if (hasException)
          System.out.println("  testSsccCheckAndDelete2Response exception " + exception );
        else
          System.out.println("  testSsccCheckAndDelete2Response result is  " + checkResult);
      }

    System.out.println("Finished testSsccCheckAndDelete2");
    return;
  }

  static public void testSsccCheckAndDelete4() throws IOException {

    System.out.println("Starting testSsccCheckAndDelete");

    Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse> callable =
        new Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndDeleteResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndDeleteResponse>();

      @Override
      public SsccCheckAndDeleteResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest.Builder builder = SsccCheckAndDeleteRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setRow(HBaseZeroCopyByteString.wrap(ROW2));
        builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
        builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
        builder.setValue(HBaseZeroCopyByteString.wrap(VALUE2));
        Delete d = new Delete(ROW2);
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

      for (SsccCheckAndDeleteResponse cresponse : result.values())
      {
        checkResult = cresponse.getResult();
        String exception = cresponse.getException();
        boolean hasException = cresponse.getHasException();
        if (hasException)
          System.out.println("  testSsccCheckAndDeleteResponse exception " + exception );
        else
          System.out.println("  testSsccCheckAndDeleteResponse result is  " + checkResult);
      }

    System.out.println("Finished testSsccCheckAndDelete");
    return;
  }

  static public void testSsccCheckAndPut() throws IOException {

    System.out.println("Starting testSsccCheckAndPut");
    final byte[] emptyVal = new byte[] {};

    Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable =
        new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndPutResponse>();

      @Override
      public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setRow(HBaseZeroCopyByteString.wrap(ROW1));
        builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
        builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
        builder.setValue(HBaseZeroCopyByteString.wrap(emptyVal));
        Put p = new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(1));
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
      for (SsccCheckAndPutResponse cresponse : result.values())
      {
        checkResult = cresponse.getResult();
        String exception = cresponse.getException();
        boolean hasException = cresponse.getHasException();
        if (hasException)
          System.out.println("  testSsccCheckAndPutResponse exception " + exception );
        else
          System.out.println("  testSsccCheckAndPutResponse result is  " + checkResult);
      }

    System.out.println("Finished testSsccCheckAndPut");
    return;
  }

  static public void testSsccCheckAndPut2() throws IOException {

    System.out.println("Starting testSsccCheckAndPut2");

    Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable =
        new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndPutResponse>();

      @Override
      public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setRow(HBaseZeroCopyByteString.wrap(ROW1));
        builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
        builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
        builder.setValue(HBaseZeroCopyByteString.wrap(VALUE1));
        Put p = new Put(ROW1).add(FAMILY, QUAL_B, Bytes.toBytes(2));
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
      for (SsccCheckAndPutResponse cresponse : result.values())
      {
        checkResult = cresponse.getResult();
        String exception = cresponse.getException();
        boolean hasException = cresponse.getHasException();
        if (hasException)
          System.out.println("  testSsccCheckAndPut2Response exception " + exception );
        else
          System.out.println("  testSsccCheckAndPut2Response result is  " + checkResult);
      }

    System.out.println("Finished testSsccCheckAndPut2");
    return;
  }

  static public void testSsccCheckAndPut3() throws IOException {

    System.out.println("Starting testSsccCheckAndPut3");

    Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable =
        new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndPutResponse>();

      @Override
      public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setRow(HBaseZeroCopyByteString.wrap(ROW1));
        builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
        builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
        builder.setValue(HBaseZeroCopyByteString.wrap(VALUE1));
        Put p = new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(1));
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
      for (SsccCheckAndPutResponse cresponse : result.values())
      {
        checkResult = cresponse.getResult();
        String exception = cresponse.getException();
        boolean hasException = cresponse.getHasException();
        if (hasException)
          System.out.println("  testSsccCheckAndPut3Response exception " + exception );
        else
          System.out.println("  testSsccCheckAndPut3Response result is  " + checkResult);
      }

    System.out.println("Finished testSsccCheckAndPut3");
    return;
  }

  static public void testSsccCheckAndPut4() throws IOException {

    System.out.println("Starting testSsccCheckAndPut4");

    Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable =
        new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndPutResponse>();

      @Override
      public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setRow(HBaseZeroCopyByteString.wrap(ROW2));
        builder.setFamily(HBaseZeroCopyByteString.wrap(FAMILY));
        builder.setQualifier(HBaseZeroCopyByteString.wrap(QUAL_A));
        builder.setValue(HBaseZeroCopyByteString.wrap(VALUE2));
        Put p = new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(2));
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
      for (SsccCheckAndPutResponse cresponse : result.values())
      {
        checkResult = cresponse.getResult();
        String exception = cresponse.getException();
        boolean hasException = cresponse.getHasException();
        if (hasException)
          System.out.println("  testSsccCheckAndPut4Response exception " + exception );
        else
          System.out.println("  testSsccCheckAndPut4Response result is  " + checkResult);
      }

    System.out.println("Finished testSsccCheckAndPut4");
    return;
  }

  static public void testSsccCloseScanner() throws IOException {

    System.out.println("Starting testSsccClosecanner");

    Batch.Call<SsccRegionService, SsccCloseScannerResponse> callable =
        new Batch.Call<SsccRegionService, SsccCloseScannerResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCloseScannerResponse> rpcCallback =
        new BlockingRpcCallback<SsccCloseScannerResponse>();

      @Override
      public SsccCloseScannerResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerRequest.Builder builder = SsccCloseScannerRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setScannerId(scannerId);
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

      for (SsccCloseScannerResponse cresponse : result.values())
      {
        boolean hasException = cresponse.getHasException();
        String exception = cresponse.getException();
        if (hasException)
          System.out.println("  testSsccCloseScannerResponse exception " + exception );
      }

    System.out.println("Finished testSsccCloseScanner");
    return;
  }

  static public void testSsccCommit() throws IOException {

    System.out.println("Starting testSsccCommit with transId: " + transId);

    try {
       bumpCommitId();
    } catch (Exception exc) {
       System.out.println("testSsccCommit : exception " + exc);
       throw new IOException("testSsccCommit : exception " + exc);
    }

    Batch.Call<SsccRegionService, SsccCommitResponse> callable =
        new Batch.Call<SsccRegionService, SsccCommitResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCommitResponse> rpcCallback =
        new BlockingRpcCallback<SsccCommitResponse>();

      @Override
      public SsccCommitResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequest.Builder builder = SsccCommitRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setCommitId(commitId);

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

      for (SsccCommitResponse cresponse : result.values())
      {
        String exception = cresponse.getException();
        boolean hasException = cresponse.getHasException();
        if (hasException)
        {
          System.out.println("SsccCommitResponse exception " + exception );
          throw new IOException(exception);
        }
      }

    System.out.println("Finished testSsccCommit");
    return;
  }

  static public void testSsccCommitRequest() throws IOException {

    System.out.println("Starting testSsccCommitRequest with transId: " + transId);

    Batch.Call<SsccRegionService, SsccCommitRequestResponse> callable =
        new Batch.Call<SsccRegionService, SsccCommitRequestResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCommitRequestResponse> rpcCallback =
        new BlockingRpcCallback<SsccCommitRequestResponse>();

      @Override
      public SsccCommitRequestResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestRequest.Builder builder = SsccCommitRequestRequest.newBuilder();
        builder.setTransactionId(transId);
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

      for (SsccCommitRequestResponse cresponse : result.values())
      {
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
        System.out.println("  SsccCommitRequestResponse value " + returnString );
      }

    System.out.println("Finished testSsccCommitRequest");
    return;
  }

  static public void testSsccCommitIfPossible() throws IOException {

    System.out.println("Starting testSsccCommitIfPossible with transId: " + transId );

    Batch.Call<SsccRegionService, SsccCommitIfPossibleResponse> callable =
        new Batch.Call<SsccRegionService, SsccCommitIfPossibleResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCommitIfPossibleResponse> rpcCallback =
        new BlockingRpcCallback<SsccCommitIfPossibleResponse>();

      @Override
      public SsccCommitIfPossibleResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleRequest.Builder builder = SsccCommitIfPossibleRequest.newBuilder();
        builder.setTransactionId(transId);
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

      for (SsccCommitIfPossibleResponse cipresponse : result.values())
      {
        boolean hasException = cipresponse.getHasException();
        if (hasException) {
          String exception = cipresponse.getException();
          System.out.println("  testSsccCommitIfPossible exception " + exception );
        }
        else {
          System.out.println("  testSsccCommitIfPossible result is " + cipresponse.getWasSuccessful());
        }

      }

    System.out.println("Finished testSsccCommitIfPossible");
    return;
  }

  static public void testSsccDelete() throws IOException {

    System.out.println("Starting testSsccDelete");

    Batch.Call<SsccRegionService, SsccDeleteTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccDeleteTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccDeleteTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccDeleteTransactionalResponse>();

      @Override
      public SsccDeleteTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalRequest.Builder builder = SsccDeleteTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Delete d = new Delete(ROW1);
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

      for (SsccDeleteTransactionalResponse dresponse : result.values())
      {
        boolean hasException = dresponse.getHasException();
        if (hasException) {
          String exception = dresponse.getException();
          System.out.println("  testSsccDelete exception " + exception );
        }
        else {
          returnStatus = dresponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccDelete returnStatus is  " + returnString);
        }

      }

    System.out.println("Finished testSsccDelete");
    return;
  }

  static public void testSsccDeleteMultiple() throws IOException {

    System.out.println("Starting testSsccDeleteMultiple");

    Batch.Call<SsccRegionService, SsccDeleteMultipleTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccDeleteMultipleTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccDeleteMultipleTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccDeleteMultipleTransactionalResponse>();

      @Override
      public SsccDeleteMultipleTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalRequest.Builder builder = SsccDeleteMultipleTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Delete d1 = new Delete(ROW1);
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, d1);
        builder.addDelete(m1);
        Delete d2 = new Delete(ROW2);
        MutationProto m2 = ProtobufUtil.toMutation(MutationType.DELETE, d2);
        builder.addDelete(m2);
        Delete d3 = new Delete(ROW3);
        MutationProto m3 = ProtobufUtil.toMutation(MutationType.DELETE, d3);
        builder.addDelete(m3);
        Delete d4 = new Delete(ROW4);
        MutationProto m4 = ProtobufUtil.toMutation(MutationType.DELETE, d4);
        builder.addDelete(m4);
        Delete d5 = new Delete(ROW5);
        MutationProto m5 = ProtobufUtil.toMutation(MutationType.DELETE, d5);
        builder.addDelete(m5);
        Delete d6 = new Delete(ROW6);
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

      for (SsccDeleteMultipleTransactionalResponse dmresponse : result.values())
      {
        boolean hasException = dmresponse.getHasException();
        if (hasException) {
          String exception = dmresponse.getException();
          System.out.println("  testSsccDeleteMultiple exception " + exception );
        }
        else {
          returnStatus = dmresponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccDeleteMultiple returnStatus is  " + returnString);
        }

      }


    System.out.println("Finished testSsccDeleteMultiple");
    return;
  }

  static public void testSsccGet() throws IOException {

    System.out.println("Starting testSsccGet");

    Batch.Call<SsccRegionService, SsccGetTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccGetTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccGetTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccGetTransactionalResponse>();

      @Override
      public SsccGetTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalRequest.Builder builder = SsccGetTransactionalRequest.newBuilder();
        //Get get = new Get(ROW1).addColumn(FAMILY, Bytes.toBytes(1));
        Get get = new Get(ROW1).addColumn(FAMILY, QUAL_A);
        builder.setGet(ProtobufUtil.toGet(get));
        builder.setTransactionId(transId);
        builder.setStartId(startId);
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

      for (SsccGetTransactionalResponse gresponse : result.values())
      {
        org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result presult = gresponse.getResult();
        Result resultFromGet = ProtobufUtil.toResult(gresponse.getResult());
        System.out.println("SsccGetTransactionalResponse Get result before action is committed:" + resultFromGet.size() + ":" + resultFromGet);
      }

    System.out.println("Finished testSsccGet");
    return;
  }

  static public void testSsccPut() throws IOException {
     testSsccPut(false);
  }

  static public void testSsccPut(final boolean statelessPut) throws IOException {

    System.out.println("Starting testSsccPut: statelessPut: " + statelessPut);

    Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutTransactionalResponse>();

      @Override
      public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(1));
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
        builder.setPut(m1);
        if (statelessPut == true) {
           builder.setIsStateless(true);
        }

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

      for (SsccPutTransactionalResponse presponse : result.values())
      {
        boolean hasException = presponse.getHasException();
        if (hasException) {
          String exception = presponse.getException();
          System.out.println("  testSsccPut exception " + exception );
        }
        else {
          returnStatus = presponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccPut returnStatus is  " + returnString);
        }

      }

    System.out.println("Finished testSsccPut");
    return;

  }

  static public void testSsccPut1() throws IOException {
     testSsccPut1(false);
  }

  static public void testSsccPut1(final boolean statelessPut) throws IOException {

    System.out.println("Starting testSsccPut1: statelessPut: " + statelessPut);

    Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutTransactionalResponse>();

      @Override
      public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(1));
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
        builder.setPut(m1);
        if (statelessPut == true) {
           builder.setIsStateless(true);
        }
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

      for (SsccPutTransactionalResponse presponse : result.values())
      {
        boolean hasException = presponse.getHasException();
        if (hasException) {
          String exception = presponse.getException();
          System.out.println("  testSsccPut1 exception " + exception );
        }
        else {
          returnStatus = presponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccPut1 returnStatus is  " + returnString);
        }

      }

    System.out.println("Finished testSsccPut1");
    return;
}

  static public void testSsccPut2() throws IOException {
     testSsccPut2(false);
  }

  static public void testSsccPut2(final boolean statelessPut) throws IOException {

    System.out.println("Starting testSsccPut2: statelessPut: " + statelessPut);

    Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutTransactionalResponse>();

      @Override
      public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(1));
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
        builder.setPut(m1);
        if (statelessPut == true) {
           builder.setIsStateless(true);
        }

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

      for (SsccPutTransactionalResponse presponse : result.values())
      {
        boolean hasException = presponse.getHasException();
        if (hasException) {
          String exception = presponse.getException();
          System.out.println("  testSsccPut2 exception " + exception );
        }
        else {
          returnStatus = presponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccPut2 returnStatus is  " + returnString);
        }

      }


    System.out.println("Finished testSsccPut2");
    return;
}

  static public void testSsccPut3() throws IOException {
     testSsccPut3(false);
  }

  static public void testSsccPut3(final boolean statelessPut) throws IOException {

    System.out.println("Starting testSsccPut3: statelessPut: " + statelessPut);

    Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutTransactionalResponse>();

      @Override
      public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW3).add(FAMILY, QUAL_A, Bytes.toBytes(1));
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
        builder.setPut(m1);
        if (statelessPut == true) {
           builder.setIsStateless(true);
        }

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

      for (SsccPutTransactionalResponse presponse : result.values())
      {
        boolean hasException = presponse.getHasException();
        if (hasException) {
          String exception = presponse.getException();
          System.out.println("  testSsccPut3 exception " + exception );
        }
        else {
          returnStatus = presponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccPut3 returnStatus is  " + returnString);
        }

      }

    System.out.println("Finished testSsccPut3");
    return;
}

  static public void testSsccPut4() throws IOException {
     testSsccPut4(false);
  }

  static public void testSsccPut4(final boolean statelessPut) throws IOException {

    System.out.println("Starting testSsccPut4: statelessPut: " + statelessPut);

    Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutTransactionalResponse>();

      @Override
      public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW4).add(FAMILY, QUAL_A, Bytes.toBytes(1));
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
        builder.setPut(m1);
        if (statelessPut == true) {
           builder.setIsStateless(true);
        }

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

      for (SsccPutTransactionalResponse presponse : result.values())
      {
        boolean hasException = presponse.getHasException();
        if (hasException) {
          String exception = presponse.getException();
          System.out.println("  testSsccPut4 exception " + exception );
        }
        else {
          returnStatus = presponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccPut4 returnStatus is  " + returnString);
        }

      }

    System.out.println("Finished testSsccPut4");
    return;
}

  static public void testSsccPut5() throws IOException {
     testSsccPut5(false);
  }

  static public void testSsccPut5(final boolean statelessPut) throws IOException {

    System.out.println("Starting testSsccPut5: statelessPut: " + statelessPut);

    Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutTransactionalResponse>();

      @Override
      public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW5).add(FAMILY, QUAL_A, Bytes.toBytes(1));
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
        builder.setPut(m1);
        if (statelessPut == true) {
           builder.setIsStateless(true);
        }

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

      for (SsccPutTransactionalResponse presponse : result.values())
      {
        boolean hasException = presponse.getHasException();
        if (hasException) {
          String exception = presponse.getException();
          System.out.println("  testSsccPut5 exception " + exception );
        }
        else {
          returnStatus = presponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccPut5 returnStatus is  " + returnString);
        }

      }

    System.out.println("Finished testSsccPut5");
    return;
}

  static public void testSsccPut6() throws IOException {
     testSsccPut6(false);
  }

  static public void testSsccPut6(final boolean statelessPut) throws IOException {

    System.out.println("Starting testSsccPut6: statelessPut: " + statelessPut);

    Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutTransactionalResponse>();

      @Override
      public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW6).add(FAMILY, QUAL_A, Bytes.toBytes(1));
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, p);
        builder.setPut(m1);
        if (statelessPut == true) {
           builder.setIsStateless(true);
        }

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

      for (SsccPutTransactionalResponse presponse : result.values())
      {
        boolean hasException = presponse.getHasException();
        if (hasException) {
          String exception = presponse.getException();
          System.out.println("  testSsccPut6 exception " + exception );
        }
        else {
          returnStatus = presponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccPut6 returnStatus is  " + returnString);
        }

      }

    System.out.println("Finished testSsccPut6");
    return;
  }

  static public void testSsccPutException() throws IOException {

    System.out.println("Starting testSsccPutException");

    Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutTransactionalResponse>();

      @Override
      public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW1).add(FAMILYBAD, QUAL_A, Bytes.toBytes(1));
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

    System.out.println("Finished testSsccPutException");
    return;
  }

  static public void testSsccPutMultiple() throws IOException {

    System.out.println("Starting testSsccPutMultiple");

    Batch.Call<SsccRegionService, SsccPutMultipleTransactionalResponse> callable =
        new Batch.Call<SsccRegionService, SsccPutMultipleTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPutMultipleTransactionalResponse> rpcCallback =
        new BlockingRpcCallback<SsccPutMultipleTransactionalResponse>();

      @Override
      public SsccPutMultipleTransactionalResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalRequest.Builder builder = SsccPutMultipleTransactionalRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Put p = new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(1));
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

      for (SsccPutMultipleTransactionalResponse pmresponse : result.values())
      {
        boolean hasException = pmresponse.getHasException();
        if (hasException) {
          String exception = pmresponse.getException();
          System.out.println("testSsccPutMultiple exception " + exception );
        }
        else {
          returnStatus = pmresponse.getStatus();
          String returnString;

          switch (returnStatus){
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
          System.out.println("  testSsccPutMultiple returnStatus is  " + returnString);
        }

      }

    System.out.println("Finished testSsccPutMultiple");
    return;
  }

  static public void testSsccPerformScan() throws IOException {

    System.out.println("Starting testSsccPerformScan");

    Batch.Call<SsccRegionService, SsccPerformScanResponse> callable =
        new Batch.Call<SsccRegionService, SsccPerformScanResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccPerformScanResponse> rpcCallback =
        new BlockingRpcCallback<SsccPerformScanResponse>();

      @Override
      public SsccPerformScanResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanRequest.Builder builder = SsccPerformScanRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));
        builder.setScannerId(scannerId);
        builder.setNumberOfRows(3);
        builder.setCloseScanner(false);
        builder.setNextCallSeq(0);

        instance.performScan(controller, builder.build(), rpcCallback);
        return rpcCallback.get();
      }
    };

    Map<byte[], SsccPerformScanResponse> presult = null;
    org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result[] results = null;

    try {
       presult = ht.coprocessorService(SsccRegionService.class, null, null, callable);
       System.out.println("  testSsccPerformScan presult size: " + presult.size() + " " + presult);
    } catch (Throwable e) {
       System.out.println("  testSsccPerformScanResponse exception getting results " + e );
    }

    int count = 0;
    boolean hasMore = false;

    org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result result = null;

    for (SsccPerformScanResponse presponse : presult.values())
    {
      if (presponse.getHasException())
      {
        String exception = presponse.getException();
        System.out.println("  testSsccPerformScanResponse exception " + exception );
      }
      else
      {
        count = presponse.getResultCount();
        results = new org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result[count];
        System.out.println("  testSsccPerformScan response count " + count + " rows ");
        for (int i = 0; i < count; i++)
        {
          result = presponse.getResult(i);
          hasMore = presponse.getHasMore();
          results[i] = result;
          result = null;
          System.out.println("  testSsccPerformScan response count " + count + ", hasMore is " + hasMore + ", result " + results[i] );
        }
      }
    }

    System.out.println("Finished testSsccPerformScan");
    return;
  }

  static public void testSsccOpenScanner() throws IOException {

    System.out.println("Starting testSsccOpenScanner");

    Batch.Call<SsccRegionService, SsccOpenScannerResponse> callable =
        new Batch.Call<SsccRegionService, SsccOpenScannerResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccOpenScannerResponse> rpcCallback =
        new BlockingRpcCallback<SsccOpenScannerResponse>();

      @Override
      public SsccOpenScannerResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerRequest.Builder builder = SsccOpenScannerRequest.newBuilder();
        builder.setTransactionId(transId);
        builder.setStartId(startId);
        builder.setRegionName(ByteString.copyFromUtf8(regionname));

        Scan scan = new Scan();
        scan.addColumn(FAMILY, Bytes.toBytes(1));

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

      for (SsccOpenScannerResponse oresponse : result.values())
      {
        scannerId = oresponse.getScannerId();
        String exception = oresponse.getException();
        boolean hasException = oresponse.getHasException();
        if (hasException)
          System.out.println("  testSsccOpenScannerResponse exception " + exception );
        else
          System.out.println("  testSsccOpenScannerResponse scannerId is " + scannerId );
      }

    System.out.println("Finished testSsccOpenScanner");
    return;
  }

  static public void testSsccRecoveryRequest() throws IOException {

    System.out.println("Starting testSsccRecoveryRequest");

    Batch.Call<SsccRegionService, SsccRecoveryRequestResponse> callable =
        new Batch.Call<SsccRegionService, SsccRecoveryRequestResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccRecoveryRequestResponse> rpcCallback =
        new BlockingRpcCallback<SsccRecoveryRequestResponse>();

      @Override
      public SsccRecoveryRequestResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestRequest.Builder rbuilder = SsccRecoveryRequestRequest.newBuilder();
        rbuilder.setTransactionId(transId);
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

      for (SsccRecoveryRequestResponse rresponse : rresult.values())
      {
        count = rresponse.getResultCount();
        l = rresponse.getResult(0);
        System.out.println("  testSsccRecoveryResponse count " + count + ", result " + l );
      }

      System.out.println("Finished testSsccRecoveryRequest");
      return;
  }

  static public void bumpTransIds() throws IOException {
     transId++;

     try {
        bumpStartId();
     } catch (IOException exc) {
        System.out.println("bumpStartId : threw exception " + exc);
        throw new IOException("bumpStartId : threw exception " + exc);
     }
  }

  static public void bumpStartId() throws IOException {

     try {
        idServer.id(ID_TM_SERVER_TIMEOUT, tmId);
        startId = tmId.val;
     } catch (IdTmException exc) {
        System.out.println("bumpStartId : IdTm threw exception " + exc);
        throw new IOException("bumpStartId : IdTm threw exception " + exc);
     }
  }

  static public void bumpCommitId() throws IOException {
     System.out.println("bumping CommitId ");

     try {
        idServer.id(ID_TM_SERVER_TIMEOUT, tmId);
        commitId = tmId.val;
     } catch (IdTmException exc) {
        System.out.println("bumpCommitId : IdTm threw exception " + exc);
        throw new IOException("bumpCommitId : IdTm threw exception " + exc);
     }
  }

  static public void main(String[] args) {

    System.out.println("Starting SsccTableClient");

    try {
      initialize();

      //Should be transaction id 1

      testSsccBeginTransaction();
      testSsccPut();
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 2

      testSsccBeginTransaction();
      testSsccDelete();
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 3

      testSsccBeginTransaction();
      testSsccPutMultiple();
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 4

      testSsccBeginTransaction();
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally

      bumpTransIds(); //Should be 5

      testSsccBeginTransaction();
      testSsccDeleteMultiple();
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 6

      testSsccBeginTransaction();
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally

      bumpTransIds(); //Should be 7

      testSsccBeginTransaction();
      testSsccPutMultiple();
      testSsccAbortTransaction();
      testSsccCommitRequest();  // should Return COMMIT_UNSUCCESSFUL 3
      try {
        testSsccCommit();      // should Return UnknownTransactionException
      } catch (IOException e) {
        System.out.println("testSsccCommit threw IOException");
        System.out.println(e.toString());
      } catch (Throwable t) {
        System.out.println("testSsccCommit threw IOException");
        System.out.println(t.toString());
      }

      bumpTransIds(); //Should be 8

      testSsccBeginTransaction();
      testSsccGet();                 // Should be no rows
      testSsccCommitIfPossible();  // Bumps id internally

      bumpTransIds(); //Should be 9 // should be COMMIT_SUCCESSFUL
      testSsccBeginTransaction();
      testSsccPut();
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 10 // should have one from the Get
      testSsccBeginTransaction();
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally

      bumpTransIds(); //Should be 11
      testSsccBeginTransaction();
      testSsccDelete();
      testSsccGet();
      testSsccCommitRequest();
      try {
        testSsccCommit();  // Should delete
      } catch (IOException e) {
        System.out.println("TestCommit from Delete/Get threw IOException");
        System.out.println(e.toString());
      } catch (Throwable t) {
        System.out.println("TestCommit from Delete/Get threw IOException");
        System.out.println(t.toString());
      }

      bumpTransIds(); //Should be 12  // Should have no rows
      testSsccBeginTransaction();
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally

      bumpTransIds(); //Should be 13
      testSsccBeginTransaction();
      testSsccCheckAndPut(); // should return true
      testSsccCheckAndPut(); // should return false
      testSsccCheckAndPut2(); // should return true
      testSsccCheckAndPut3(); // should return Exception
      testSsccCommitIfPossible();  // Bumps id internally // should return true

      bumpTransIds(); //Should be 14
      testSsccBeginTransaction();
      testSsccGet();             // Should have a row
      testSsccCommitIfPossible();  // Bumps id internally // should return true

      bumpTransIds(); //Should be 15
      testSsccBeginTransaction();
      testSsccCheckAndDelete2(); // should return true
      testSsccCommitIfPossible();  // Bumps id internally


      bumpTransIds(); //Should be 16
      testSsccBeginTransaction();
      testSsccGet();             // Should have one row left
      testSsccCommitIfPossible();  // Bumps id internally


      bumpTransIds(); //Should be 17
      testSsccBeginTransaction();
      testSsccDeleteMultiple();
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 18
      testSsccBeginTransaction();
      testSsccCheckAndPut(); // should return true
      testSsccGet();         // should have 1 row
      testSsccCheckAndDelete(); // should return true
      testSsccGet();         // should have no rows
      testSsccCommitIfPossible();  // Bumps id internally


      bumpTransIds(); //Should be 19
      testSsccBeginTransaction();
      testSsccGet();              // should have no rows
      testSsccCommitIfPossible();  // Bumps id internally


      bumpTransIds(); //Should be 20  // should be COMMIT_SUCCESSFUL with six rows
      testSsccBeginTransaction();
      testSsccPut1();
      testSsccPut2();
      testSsccPut3();
      testSsccPut4();
      testSsccPut5();
      testSsccPut6();

      System.out.println("testSsccOpenScanner open a new scanner");
      testSsccOpenScanner();
      System.out.println("TestSsccPerformScan get rows 1 through 3");
      testSsccPerformScan();  // Get the first three
      System.out.println("TestSsccPerformScan get rows 4 through 6");
      testSsccPerformScan();  // Get the second three
      testSsccCloseScanner();
      testSsccCommitIfPossible();  // Bumps id internally


      bumpTransIds(); //Should be 21  Should show ROW1
      testSsccBeginTransaction();
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally


      bumpTransIds(); //Should be 22
      testSsccBeginTransaction();
      testSsccDeleteMultiple();
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 23  Should show zero rows
      testSsccBeginTransaction();
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally


/*
      testRecoveryRequest();
*/
      // Exception testing

      // Try to abort a transaction that doesn't exist
      // Should catch IOException

      try {
        testSsccAbortTransaction();
      } catch (IOException e) {
        System.out.println("TestAbortTransaction threw IOException");
        System.out.println(e.toString());
      } catch (Throwable t) {
        System.out.println("SsccAbortTransaction threw throwable exception");
        System.out.println(t.toString());
      }

      // Try to commit a Put that uses an invalid FAMILY
      // Should catch NoSuchColumnFamilyException

      bumpTransIds(); // Should be 24
      testSsccBeginTransaction();
      testSsccPutException();
      testSsccCommitRequest();
      try {
        testSsccCommit();
      } catch (IOException e) {
        System.out.println("TestPutException threw IOException");
        System.out.println(e.toString());
      } catch (Throwable t) {
        System.out.println("SsccPutException threw throwable exception");
        System.out.println(t.toString());
      }

      // Confirm there are no records in 'table1'
      bumpTransIds(); //Should be 25
      testSsccBeginTransaction();
      System.out.println("Should show no matching rows from the Get");
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally


      bumpTransIds(); //Should be 26, leaves no rows in the table
      testSsccBeginTransaction();
      testSsccPut();
      testSsccGet();
      testSsccDelete();
      testSsccGet();
      testSsccCommitRequest();
      testSsccCommit();

      // Confirm there are no records in 'table1'
      bumpTransIds(); //Should be 27
      testSsccBeginTransaction();
      System.out.println("Should show no matching rows from the Get");
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally


      //Should be transaction id 28
      bumpTransIds();
      testSsccBeginTransaction();
      testSsccPut(true);      // stateless put
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 29 // should be COMMIT_SUCCESSFUL
      testSsccBeginTransaction();
      testSsccPut(true);      // stateless put
      testSsccCommitRequest();
      testSsccCommit();

      bumpTransIds(); //Should be 30  // should be COMMIT_SUCCESSFUL with six rows
      testSsccBeginTransaction();
      testSsccPut1(true);
      testSsccPut2(true);
      testSsccPut3(true);
      testSsccPut4(true);
      testSsccPut5(true);
      testSsccPut6(true);

      System.out.println("testSsccOpenScanner open a new scanner");
      testSsccOpenScanner();
      System.out.println("TestSsccPerformScan get rows 1 through 3");
      testSsccPerformScan();  // Get the first three
      System.out.println("TestSsccPerformScan get rows 4 through 6");
      testSsccPerformScan();  // Get the second three
      testSsccCloseScanner();
      testSsccCommitIfPossible();  // Bumps id internally


      bumpTransIds(); //Should be 31, leaves no rows in the table
      testSsccBeginTransaction();
      testSsccPut(true);
      testSsccGet();
      testSsccDelete();
      testSsccGet();
      testSsccCommitRequest();
      testSsccCommit();

      // Confirm there are no records in 'table1'
      bumpTransIds(); //Should be 32
      testSsccBeginTransaction();
      System.out.println("Should show no matching rows from the Get");
      testSsccGet();
      testSsccCommitIfPossible();  // Bumps id internally


    } catch (IOException e) {
      System.out.println("SsccTableClient threw IOException: " + e);
      System.out.println(e.toString());
    } catch (Throwable t) {
      System.out.println("SsccTableClient threw throwable exception: " + t);
      System.out.println(t.toString());
    }

    System.out.println("Finished SsccTableClient");
  }

}

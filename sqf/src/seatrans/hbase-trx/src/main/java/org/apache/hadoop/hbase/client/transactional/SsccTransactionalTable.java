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

package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.io.InterruptedIOException;
import org.apache.hadoop.hbase.client.RetriesExhaustedWithDetailsException;

import org.apache.hadoop.hbase.client.HConnection;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto.MutationType;
import org.apache.hadoop.hbase.regionserver.transactional.SingleVersionDeleteNotSupported;
import org.apache.hadoop.hbase.client.HConnectionManager;
import org.apache.hadoop.hbase.util.Bytes;

import com.google.protobuf.ByteString;
import com.google.protobuf.HBaseZeroCopyByteString;
import com.google.protobuf.ServiceException;

import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRegionService;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccTransactionalAggregateRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccTransactionalAggregateResponse;

import org.apache.hadoop.hbase.client.transactional.SsccUpdateConflictException;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;


/**
 * Table with transactional support.
 */
public class SsccTransactionalTable extends HTable implements TransactionalTableClient {
    static final Log LOG = LogFactory.getLog(RMInterface.class);
    /*   // Not necessary for 0.98
    private RpcRetryingCallerFactory rpcCallerFactory;    
    private RpcControllerFactory rpcControllerFactory;
    */
    static private HConnection connection = null;
    static Configuration       config = HBaseConfiguration.create();
    static ExecutorService     threadPool;

    static {
        config.set("hbase.hregion.impl", "org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion");
        try {
            connection = HConnectionManager.createConnection(config);
        }
        catch (IOException ioe) {
            LOG.error("Exception on TransactionTable anonymous static method. IOException while creating HConnection");
        }
        threadPool = Executors.newCachedThreadPool();
   }

    /**
     * @param conf
     * @param tableName
     * @throws IOException
     */
    public SsccTransactionalTable( final String tableName) throws IOException {
        this(Bytes.toBytes(tableName));        
    }

    /**
     * @param conf
     * @param tableName
     * @throws IOException
     */
    public SsccTransactionalTable( final byte[] tableName) throws IOException {
        super( tableName,connection, threadPool);       
    }

    public void resetConnection() throws IOException {
        if (LOG.isDebugEnabled()) LOG.debug("Resetting connection for " + this.getTableDescriptor().getTableName());
        HConnection conn = this.getConnection();
        conn = HConnectionManager.createConnection(this.getConfiguration());
    }


    private void addLocation(final TransactionState transactionState, HRegionLocation location) {
      if (LOG.isTraceEnabled()) LOG.trace("addLocation ENTRY");
      if (transactionState.addRegion(location))
         if (LOG.isTraceEnabled()) LOG.trace("TransactionalTable.recordServer added region to TS. Beginning txn " + transactionState + " on server");
      if (LOG.isTraceEnabled()) LOG.trace("addLocation EXIT");
    }

    /**
     * Method for getting data from a row
     * 
     * @param get the Get to fetch
     * @return the result
     * @throws IOException
     * @since 0.20.0
     */
    public Result get(final TransactionState transactionState, final Get get) throws IOException {

       return get(transactionState, get, true);

    }

    public Result get(final TransactionState transactionState, final Get get, final boolean bool_addLocation) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.get");

      if (bool_addLocation) addLocation(transactionState, super.getRegionLocation(get.getRow()));
          final String regionName = super.getRegionLocation(get.getRow()).getRegionInfo().getRegionNameAsString();
          Batch.Call<SsccRegionService, SsccGetTransactionalResponse> callable = 
          new Batch.Call<SsccRegionService, SsccGetTransactionalResponse>() {
            ServerRpcController controller = new ServerRpcController();
            BlockingRpcCallback<SsccGetTransactionalResponse> rpcCallback = 
            new BlockingRpcCallback<SsccGetTransactionalResponse>();

            @Override
            public SsccGetTransactionalResponse call(SsccRegionService instance) throws IOException {
            org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalRequest.Builder builder = SsccGetTransactionalRequest.newBuilder();            
            builder.setGet(ProtobufUtil.toGet(get));
            builder.setTransactionId(transactionState.getTransactionId());
            builder.setRegionName(ByteString.copyFromUtf8(regionName));

            instance.get(controller, builder.build(), rpcCallback);
            return rpcCallback.get();
           }
          };

          Map<byte[], SsccGetTransactionalResponse> result = null;
          try {
            result = super.coprocessorService(SsccRegionService.class, get.getRow(), get.getRow(), callable);
          } catch (Throwable e) {
            e.printStackTrace();
            throw new IOException("ERROR while calling coprocessor get");
          }            
          Collection<SsccGetTransactionalResponse> results = result.values();
          // Should only be one result, if more than one. Can't handle.
          // Need to test whether '!=' or '>' is correct
          if (LOG.isTraceEnabled()) LOG.trace("Results count: " + results.size());
          //if(results.size() != 1)
          //  throw new IOException("Incorrect number of results from coprocessor call");
          SsccGetTransactionalResponse[] resultArray = new SsccGetTransactionalResponse[results.size()];
          results.toArray(resultArray);
          if(resultArray.length == 0) 
              throw new IOException("Problem with calling coprocessor get, no regions returned result");

          if(resultArray[0].hasException())
            throw new IOException(resultArray[0].getException());
          return ProtobufUtil.toResult(resultArray[0].getResult());
    }

    /**
     * @param delete
     * @throws IOException
     * @since 0.20.0
     */
    public void delete(final TransactionState transactionState, final Delete delete) throws IOException {

       delete(transactionState, delete, true);

    }

    public void delete(final TransactionState transactionState, final Delete delete, final boolean bool_addLocation) throws IOException {
        SingleVersionDeleteNotSupported.validateDelete(delete);
        if (bool_addLocation) addLocation(transactionState, super.getRegionLocation(delete.getRow()));
        final String regionName = super.getRegionLocation(delete.getRow()).getRegionInfo().getRegionNameAsString();
            Batch.Call<SsccRegionService, SsccDeleteTransactionalResponse> callable =
                new Batch.Call<SsccRegionService, SsccDeleteTransactionalResponse>() {
              ServerRpcController controller = new ServerRpcController();
              BlockingRpcCallback<SsccDeleteTransactionalResponse> rpcCallback =
                new BlockingRpcCallback<SsccDeleteTransactionalResponse>();

              @Override
              public SsccDeleteTransactionalResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalRequest.Builder builder = SsccDeleteTransactionalRequest.newBuilder();      
                builder.setTransactionId(transactionState.getTransactionId());
                builder.setRegionName(ByteString.copyFromUtf8(regionName));

                MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, delete);
                builder.setDelete(m1);

                instance.delete(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
              }
            };

            byte[] row = delete.getRow();
            Map<byte[], SsccDeleteTransactionalResponse> result = null; 
            try {
              result = super.coprocessorService(SsccRegionService.class, row, row, callable);

            } catch (ServiceException e) {
              e.printStackTrace();
              throw new IOException();
            } catch (Throwable t) {
              t.printStackTrace();
              throw new IOException();
            } 
            Collection<SsccDeleteTransactionalResponse> results = result.values();
            //GetTransactionalResponse[] resultArray = (GetTransactionalResponse[]) results.toArray();
            SsccDeleteTransactionalResponse[] resultArray = new SsccDeleteTransactionalResponse[results.size()];
            results.toArray(resultArray);
            if(resultArray.length == 0)
              throw new IOException("Problem with calling coprocessor delete, no regions returned result");

            if(resultArray[0].hasException())
              throw new IOException(resultArray[0].getException());
          }

    /**
     * Commit a Put to the table.
     * <p>
     * If autoFlush is false, the update is buffered.
     * 
     * @param put
     * @throws IOException
     * @since 0.20.0
     */
    public synchronized void put(final TransactionState transactionState, final Put put) throws IOException {

      put(transactionState, put, true);

    }

    public synchronized void put(final TransactionState transactionState, final Put put, final boolean bool_addLocation) throws IOException{
      validatePut(put);
      if (LOG.isTraceEnabled()) LOG.trace("TransactionalTable.put ENTRY");

      if (bool_addLocation) addLocation(transactionState, super.getRegionLocation(put.getRow()));
      final String regionName = super.getRegionLocation(put.getRow()).getRegionInfo().getRegionNameAsString();
          Batch.Call<SsccRegionService, SsccPutTransactionalResponse> callable =
          new Batch.Call<SsccRegionService, SsccPutTransactionalResponse>() {
          ServerRpcController controller = new ServerRpcController();
          BlockingRpcCallback<SsccPutTransactionalResponse> rpcCallback =
              new BlockingRpcCallback<SsccPutTransactionalResponse>();
          @Override
          public SsccPutTransactionalResponse call(SsccRegionService instance) throws IOException {
            org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest.Builder builder = SsccPutTransactionalRequest.newBuilder();
            builder.setTransactionId(transactionState.getTransactionId());
            builder.setRegionName(ByteString.copyFromUtf8(regionName));

            MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, put);
            builder.setPut(m1);

            instance.put(controller, builder.build(), rpcCallback);
            return rpcCallback.get();
          }
        };
        Map<byte[], SsccPutTransactionalResponse> result = null;
        try {
          result = super.coprocessorService(SsccRegionService.class, put.getRow(), put.getRow(), callable);
        } catch (Throwable e) {
          e.printStackTrace();
          throw new IOException("ERROR while calling coprocessor put");
        }
        Collection<SsccPutTransactionalResponse> results = result.values();
        SsccPutTransactionalResponse[] resultArray = new SsccPutTransactionalResponse[results.size()]; 
        results.toArray(resultArray);
        if(resultArray.length == 0) 
          throw new IOException("Problem with calling coprocessor put, no regions returned result");

        if(resultArray[0].getStatus() != 1) //TODO: replace the hardcode 1 to a MACRO or something
          throw new IOException("conflict writing");
        if(resultArray[0].hasException())
          throw new IOException(resultArray[0].getException());     
    // put is void, may not need to check result
    if (LOG.isTraceEnabled()) LOG.trace("TransactionalTable.put EXIT");
  }

  public synchronized ResultScanner getScanner(final TransactionState transactionState, final Scan scan) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.getScanner");
    if (scan.getCaching() <= 0) {
        scan.setCaching(getScannerCaching());
    }

    Long value = (long) 0;
    SsccTransactionalScanner scanner = new SsccTransactionalScanner(this, transactionState, scan, value);

    return scanner;
  }

  public boolean checkAndDelete(final TransactionState transactionState, final byte[] row, final byte[] family,
           final byte[] qualifier, final byte[] value, final Delete delete) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.checkAndDelete row: " + row + " family: " + family + " qualifier: " + qualifier + " value: " + value);
    if (!Bytes.equals(row, delete.getRow())) {
            throw new IOException("Action's getRow must match the passed row");
    }
    final String regionName = super.getRegionLocation(delete.getRow()).getRegionInfo().getRegionNameAsString();
    if(regionName == null) 
       throw new IOException("Null regionName");
    Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse> callable =
      new Batch.Call<SsccRegionService, SsccCheckAndDeleteResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndDeleteResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndDeleteResponse>();

      @Override
      public SsccCheckAndDeleteResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest.Builder builder = SsccCheckAndDeleteRequest.newBuilder();
        builder.setTransactionId(transactionState.getTransactionId());
        builder.setRegionName(ByteString.copyFromUtf8(regionName));
        builder.setRow(HBaseZeroCopyByteString.wrap(row));
        if(family != null)
        	builder.setFamily(HBaseZeroCopyByteString.wrap(family));
        else
        	builder.setFamily(HBaseZeroCopyByteString.wrap(new byte[]{}));

        if(qualifier != null)
        	builder.setQualifier(HBaseZeroCopyByteString.wrap(qualifier));
        else
        	builder.setQualifier(HBaseZeroCopyByteString.wrap(new byte[]{}));
        if(value != null)
        	builder.setValue(HBaseZeroCopyByteString.wrap(value));
        else 
        	builder.setValue(HBaseZeroCopyByteString.wrap(new byte[]{}));

        MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, delete);
        builder.setDelete(m1);

        instance.checkAndDelete(controller, builder.build(), rpcCallback);
        return rpcCallback.get();
        }
      };

      Map<byte[], SsccCheckAndDeleteResponse> result = null;
      try {
        result = super.coprocessorService(SsccRegionService.class, delete.getRow(), delete.getRow(), callable);
      } catch (Throwable e) {
        e.printStackTrace();
        throw new IOException("ERROR while calling coprocessor checkAndDelete");
      }

      Collection<SsccCheckAndDeleteResponse> results = result.values();
      SsccCheckAndDeleteResponse[] resultArray = new SsccCheckAndDeleteResponse[results.size()];
      results.toArray(resultArray);

      if(resultArray.length == 0) 
         throw new IOException("Problem with calling coprocessor checkAndDelete, no regions returned result");
      // Should only be one result, if more than one. Can't handle.
      return resultArray[0].getResult();

    // TODO: Need to fix the exception checking and return, but for now need it
    //       to work so commenting out 

      /*
      if(results.size() != 1)
        throw new IOException("Incorrect number of results from coprocessor call");
      CheckAndDeleteResponse[] resultArray = (CheckAndDeleteResponse[]) results.toArray();
      if (resultArray[0].hasException())
        throw new IOException(resultArray[0].getException());
        */
   }
	public boolean checkAndPut(final TransactionState transactionState,
			final byte[] row, final byte[] family, final byte[] qualifier,
			final byte[] value, final Put put) throws IOException {

		if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.checkAndPut row: " + row
				+ " family: " + family + " qualifier: " + qualifier
				+ " value: " + value);
		if (!Bytes.equals(row, put.getRow())) {
			throw new IOException("Action's getRow must match the passed row");
		}
		final String regionName = super.getRegionLocation(put.getRow()).getRegionInfo().getRegionNameAsString();
		if (LOG.isTraceEnabled()) LOG.trace("checkAndPut, region name: " + regionName);
	    //if(regionName == null) 
	    	//throw new IOException("Null regionName");
	    //System.out.println("RegionName: " + regionName);

		Batch.Call<SsccRegionService, SsccCheckAndPutResponse> callable =
        new Batch.Call<SsccRegionService, SsccCheckAndPutResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<SsccCheckAndPutResponse> rpcCallback =
        new BlockingRpcCallback<SsccCheckAndPutResponse>();

      @Override
      public SsccCheckAndPutResponse call(SsccRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest.Builder builder = SsccCheckAndPutRequest.newBuilder();
        builder.setTransactionId(transactionState.getTransactionId());
        builder.setRegionName(ByteString.copyFromUtf8(regionName));
        builder.setRow(HBaseZeroCopyByteString.wrap(row));
        if (family != null)
        	builder.setFamily(HBaseZeroCopyByteString.wrap(family));
        else 
        	builder.setFamily(HBaseZeroCopyByteString.wrap(new byte[]{}));
        if (qualifier != null )
        	builder.setQualifier(HBaseZeroCopyByteString.wrap(qualifier));
        else 
        	builder.setQualifier(HBaseZeroCopyByteString.wrap(new byte[]{}));
        if (value != null)
        	builder.setValue(HBaseZeroCopyByteString.wrap(value));
        else
        	builder.setValue(HBaseZeroCopyByteString.wrap(new byte[]{}));
        //Put p = new Put(ROW1);
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, put);
        builder.setPut(m1);

        instance.checkAndPut(controller, builder.build(), rpcCallback);
        return rpcCallback.get();
      }
    };

      Map<byte[], SsccCheckAndPutResponse> result = null;
      try {
         result = super.coprocessorService(SsccRegionService.class, put.getRow(), put.getRow(), callable);
      } catch (Throwable e) {
          StringWriter sw = new StringWriter();
          PrintWriter pw = new PrintWriter(sw);
          e.printStackTrace(pw);
          //sw.toString();
        throw new IOException("ERROR while calling coprocessor checkAndDelete" + sw.toString());
      }
      Collection<SsccCheckAndPutResponse> results = result.values();
      // Should only be one result, if more than one. Can't handle.
      SsccCheckAndPutResponse[] resultArray = new SsccCheckAndPutResponse[results.size()];
      results.toArray(resultArray);
      if(resultArray.length == 0)
         throw new IOException("Problem with calling checkAndDelete in coprocessor, no regions returned result");
      return resultArray[0].getResult();
    }

       /**
   	 * Looking forward to TransactionalRegion-side implementation
   	 * 
   	 * @param transactionState
   	 * @param deletes
   	 * @throws IOException
   	 */
     public void delete(final TransactionState transactionState, List<Delete> deletes) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.delete[] row ");

        // collect all rows from same region
        final Map<TransactionRegionLocation, List<Delete>> rows = new HashMap<TransactionRegionLocation, List<Delete>>();
        HRegionLocation hlocation = null;
        TransactionRegionLocation location = null;
   			List<Delete> list = null;
   			for (Delete del : deletes) {
                                hlocation = this.getRegionLocation(del.getRow(), false);
                                location = new TransactionRegionLocation(hlocation.getRegionInfo(), hlocation.getServerName());
   				if (!rows.containsKey(location)) {
   					list = new ArrayList<Delete>();
   					rows.put(location, list);
   				} else {
   					list = rows.get(location);
   				}
   				list.add(del);
   			}

   			final List<Delete> rowsInSameRegion = new ArrayList<Delete>();
                        for (Map.Entry<TransactionRegionLocation, List<Delete>> entry : rows.entrySet()) {
   				rowsInSameRegion.clear();
   				rowsInSameRegion.addAll(entry.getValue());
   				final String regionName = entry.getKey().getRegionInfo().getRegionNameAsString();
   			 Batch.Call<SsccRegionService, SsccDeleteMultipleTransactionalResponse> callable =
   	        new Batch.Call<SsccRegionService, SsccDeleteMultipleTransactionalResponse>() {
   	      ServerRpcController controller = new ServerRpcController();
   	      BlockingRpcCallback<SsccDeleteMultipleTransactionalResponse> rpcCallback =
   	        new BlockingRpcCallback<SsccDeleteMultipleTransactionalResponse>();

   	      @Override
   	      public SsccDeleteMultipleTransactionalResponse call(SsccRegionService instance) throws IOException {
   	        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalRequest.Builder builder = SsccDeleteMultipleTransactionalRequest.newBuilder();
   	        builder.setTransactionId(transactionState.getTransactionId());
   	        builder.setRegionName(ByteString.copyFromUtf8(regionName));

   	        for(Delete delete : rowsInSameRegion) {
   	          MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, delete);
   	          builder.addDelete(m1);
   	        }

   	        instance.deleteMultiple(controller, builder.build(), rpcCallback);
   	        return rpcCallback.get();
   	      }
   	    };

   	   Map<byte[], SsccDeleteMultipleTransactionalResponse> result = null;
 	      try {
                 result = super.coprocessorService(SsccRegionService.class,
                                      entry.getKey().getRegionInfo().getStartKey(),
                                      TransactionManager.binaryIncrementPos(entry.getKey().getRegionInfo().getEndKey(),-1),
                                      callable);
 	      } catch (Throwable e) {
 	        e.printStackTrace();
	        throw new IOException("ERROR while calling coprocessor delete");
 	      }
	      if(result.size() > 1) {
             LOG.error("result size for multiple delete:" + result.size());
             throw new IOException("Incorrect number of region results in delete");
	      }
	      Collection<SsccDeleteMultipleTransactionalResponse> results = result.values();
	      SsccDeleteMultipleTransactionalResponse[] resultArray = new SsccDeleteMultipleTransactionalResponse[results.size()];
	      results.toArray(resultArray);
	      if(resultArray.length == 0) 
                throw new IOException("Problem with calling coprocessor delete, no regions returned result");
          if (resultArray[0].hasException())
             throw new IOException(resultArray[0].getException());
        }
       }

   	/**
	 * Put a set of rows
	 * 
	 * @param transactionState
	 * @param puts
	 * @throws IOException
	 */
	public void put(final TransactionState transactionState,
			final List<Put> puts) throws IOException {
		if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.put[] row ");
		// collect all rows from same region
		final Map<TransactionRegionLocation, List<Put>> rows = new HashMap<TransactionRegionLocation, List<Put>>();
		HRegionLocation hlocation = null;
                TransactionRegionLocation location = null;
		List<Put> list = null;
		for (Put put : puts) {
			validatePut(put);
			hlocation = this.getRegionLocation(put.getRow(), false);
                        location = new TransactionRegionLocation(hlocation.getRegionInfo(), hlocation.getServerName());
			if (!rows.containsKey(location)) {
				list = new ArrayList<Put>();
				rows.put(location, list);
			} else {
				list = rows.get(location);
			}
			list.add(put);
		}

		final List<Put> rowsInSameRegion = new ArrayList<Put>();
		for (Map.Entry<TransactionRegionLocation, List<Put>> entry : rows.entrySet()) {
			rowsInSameRegion.clear();
			rowsInSameRegion.addAll(entry.getValue());
			final String regionName = entry.getKey().getRegionInfo().getRegionNameAsString();
			Batch.Call<SsccRegionService, SsccPutMultipleTransactionalResponse> callable =
	        new Batch.Call<SsccRegionService, SsccPutMultipleTransactionalResponse>() {
	      ServerRpcController controller = new ServerRpcController();
	      BlockingRpcCallback<SsccPutMultipleTransactionalResponse> rpcCallback =
	        new BlockingRpcCallback<SsccPutMultipleTransactionalResponse>();

	      @Override
	      public SsccPutMultipleTransactionalResponse call(SsccRegionService instance) throws IOException {
	        org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalRequest.Builder builder = SsccPutMultipleTransactionalRequest.newBuilder();
	        builder.setTransactionId(transactionState.getTransactionId());
	        builder.setRegionName(ByteString.copyFromUtf8(regionName));

	        for (Put put : rowsInSameRegion){
	          MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, put);
	          builder.addPut(m1);
	        }

	        instance.putMultiple(controller, builder.build(), rpcCallback);
	        return rpcCallback.get();
	      }
	    };
	    Map<byte[], SsccPutMultipleTransactionalResponse> result = null;
      try {
        result = super.coprocessorService(SsccRegionService.class, 
        								  entry.getKey().getRegionInfo().getStartKey(), 
        								  TransactionManager.binaryIncrementPos(entry.getKey().getRegionInfo().getEndKey(),-1), 
        								  callable);
      } catch (Throwable e) {
        e.printStackTrace();
        throw new IOException("ERROR while calling coprocessor put");
      }
      Collection<SsccPutMultipleTransactionalResponse> results = result.values();
      SsccPutMultipleTransactionalResponse[] resultArray = new SsccPutMultipleTransactionalResponse[results.size()];
      results.toArray(resultArray);
      if(resultArray.length == 0)
         throw new IOException("Problem with calling coprocessor put, no regions returned result");

      if (resultArray[0].hasException()) 
        throw new IOException(resultArray[0].getException());
     }

	}
	
	// validate for well-formedness
	public void validatePut(final Put put) throws IllegalArgumentException {
		if (put.isEmpty()) {
			throw new IllegalArgumentException("No columns to insert");
		}
		if (maxKeyValueSize > 0) {
			for (List<KeyValue> list : put.getFamilyMap().values()) {
				for (KeyValue kv : list) {
					if (kv.getLength() > maxKeyValueSize) {
						throw new IllegalArgumentException(
								"KeyValue size too large");
					}
				}
			}
		}
	}
	public HRegionLocation getRegionLocation(byte[] row, boolean f)
                                  throws IOException {
        return super.getRegionLocation(row, f);
    }
    public void close()  throws IOException { super.close(); }

    public void setAutoFlush(boolean autoFlush, boolean b)
    {
        super.setAutoFlush(autoFlush,b);
    }
    public org.apache.hadoop.conf.Configuration getConfiguration()
    {
        return super.getConfiguration();
    }
    public void flushCommits()
                  throws InterruptedIOException,
                RetriesExhaustedWithDetailsException {
         super.flushCommits();
    }
    public HConnection getConnection()
    {
        return super.getConnection();
    }
    public byte[][] getEndKeys()
                    throws IOException
    {
        return super.getEndKeys();
    }
    public byte[][] getStartKeys() throws IOException
    {
        return super.getStartKeys();
    }
    public void setWriteBufferSize(long writeBufferSize) throws IOException
    {
        super.setWriteBufferSize(writeBufferSize);
    }
    public long getWriteBufferSize()
    {
        return super.getWriteBufferSize();
    }
    public byte[] getTableName()
    {
        return super.getTableName();
    }
    public ResultScanner getScanner(Scan scan) throws IOException
    {
        return super.getScanner(scan);
    }
    public Result get(Get g) throws IOException
    {
        return super.get(g);
    }
    public Result[] get( List<Get> g) throws IOException
    {
        return super.get(g);
    }
    public void delete(Delete d) throws IOException
    {
        super.delete(d);
    }
    public void delete(List<Delete> deletes) throws IOException
    {
        super.delete(deletes);
    }
    public boolean checkAndPut(byte[] row, byte[] family, byte[] qualifier, byte[] value, Put put) throws IOException
    {
        return super.checkAndPut(row,family,qualifier,value,put);
    }
    public void put(Put p) throws  InterruptedIOException,RetriesExhaustedWithDetailsException
    {
        super.put(p);
    }
    public void put(List<Put> p) throws  InterruptedIOException,RetriesExhaustedWithDetailsException
    {
        super.put(p);
    }
    public boolean checkAndDelete(byte[] row, byte[] family, byte[] qualifier, byte[] value,  Delete delete) throws IOException
    {
        return super.checkAndDelete(row,family,qualifier,value,delete);
    }
	private int maxKeyValueSize;
	
}

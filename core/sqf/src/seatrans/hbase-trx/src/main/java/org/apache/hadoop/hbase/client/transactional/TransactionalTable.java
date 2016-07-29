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


package org.apache.hadoop.hbase.client.transactional;

import java.io.File;
import java.util.Collection;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.apache.hadoop.hbase.client.RetriesExhaustedWithDetailsException;

import org.apache.commons.codec.binary.Hex;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.KeyValueUtil;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.TrafParallelClientScanner;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndDeleteRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndDeleteResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndPutRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndPutResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.GetTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.GetTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto.MutationType;
import org.apache.hadoop.hbase.regionserver.transactional.SingleVersionDeleteNotSupported;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.fs.Path;

import com.google.protobuf.ByteString;
import com.google.protobuf.HBaseZeroCopyByteString;
import com.google.protobuf.ServiceException;


/**
 * Table with transactional support.
 */
public class TransactionalTable extends HTable implements TransactionalTableClient {
    static final Log LOG = LogFactory.getLog(RMInterface.class);
    static private Connection connection = null;
    static ExecutorService     threadPool;
    static int                 retries = 15;
    static int                 delay = 1000;
    private String retryErrMsg = "Coprocessor result is null, retries exhausted";

    /**
     * @param tableName
     * @throws IOException
     */
    public TransactionalTable(final String tableName, Connection conn) throws IOException {
        this(Bytes.toBytes(tableName), conn);        
    }

    /**
     * @param tableName
     * @throws IOException
     */
    public TransactionalTable(final byte[] tableName, Connection conn) throws IOException {
       super(tableName, conn, threadPool);     
       this.connection = conn; 
    }

    private void addLocation(final TransactionState transactionState, HRegionLocation location) {
      if (LOG.isTraceEnabled()) LOG.trace("addLocation ENTRY");
      if (transactionState.addRegion(location)){
         if (LOG.isTraceEnabled()) LOG.trace("addLocation added region [" + location.getRegionInfo().getRegionNameAsString() + " endKey: "
                     + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " to TS. Beginning txn " + transactionState.getTransactionId() + " on server");
      }
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
      Batch.Call<TrxRegionService, GetTransactionalResponse> callable = 
      new Batch.Call<TrxRegionService, GetTransactionalResponse>() {
        ServerRpcController controller = new ServerRpcController();
        BlockingRpcCallback<GetTransactionalResponse> rpcCallback = 
        new BlockingRpcCallback<GetTransactionalResponse>();    

        @Override
        public GetTransactionalResponse call(TrxRegionService instance) throws IOException {    
        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.GetTransactionalRequest.Builder builder = GetTransactionalRequest.newBuilder();            
        builder.setGet(ProtobufUtil.toGet(get));
        builder.setTransactionId(transactionState.getTransactionId());
        builder.setRegionName(ByteString.copyFromUtf8(regionName));
   
        instance.get(controller, builder.build(), rpcCallback);
        return rpcCallback.get();    
       }   
      };  

      GetTransactionalResponse result = null;   
      try {
        int retryCount = 0;
        boolean retry = false;
        do {
          Iterator<Map.Entry<byte[], TrxRegionProtos.GetTransactionalResponse>> it = super.coprocessorService(TrxRegionService.class, 
                                                                                                              get.getRow(), 
                                                                                                              get.getRow(), 
                                                                                                              callable)
                                                                                                              .entrySet().iterator();
          if(it.hasNext()) {
            result = it.next().getValue();
            retry = false;
          } 

          if(result == null || result.getException().contains("closing region")) {
            Thread.sleep(TransactionalTable.delay);
            retry = true;
            transactionState.setRetried(true);
            retryCount++;
          }
        } while (retryCount < TransactionalTable.retries && retry == true);
      } catch (Throwable e) {
        throw new IOException("ERROR while calling coprocessor", e);
      } 
      if(result == null)
        throw new IOException(retryErrMsg);
      else if(result.hasException())
        throw new IOException(result.getException());
      return ProtobufUtil.toResult(result.getResult());      
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
        
        Batch.Call<TrxRegionService, DeleteTransactionalResponse> callable =
            new Batch.Call<TrxRegionService, DeleteTransactionalResponse>() {
          ServerRpcController controller = new ServerRpcController();
          BlockingRpcCallback<DeleteTransactionalResponse> rpcCallback =
            new BlockingRpcCallback<DeleteTransactionalResponse>();

          @Override
          public DeleteTransactionalResponse call(TrxRegionService instance) throws IOException {
            org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteTransactionalRequest.Builder builder = DeleteTransactionalRequest.newBuilder();      
            builder.setTransactionId(transactionState.getTransactionId());
            builder.setRegionName(ByteString.copyFromUtf8(regionName));
            
            MutationProto m1 = ProtobufUtil.toMutation(MutationType.DELETE, delete);
            builder.setDelete(m1);

            instance.delete(controller, builder.build(), rpcCallback);
            return rpcCallback.get();
          }
        };

        byte[] row = delete.getRow();
        DeleteTransactionalResponse result = null; 
        try {
          int retryCount = 0;
          boolean retry = false;
          do {
            Iterator<Map.Entry<byte[], DeleteTransactionalResponse>> it = super.coprocessorService(TrxRegionService.class, 
                                              row, 
                                              row, 
                                              callable)
                                              .entrySet().iterator();
            if(it.hasNext()) {
              result = it.next().getValue();
              retry = false;
            }

            if(result == null || result.getException().contains("closing region")) {
              Thread.sleep(TransactionalTable.delay);
              retry = true;
              transactionState.setRetried(true);
              retryCount++;
            }
          } while (retryCount < TransactionalTable.retries && retry == true);
        } catch (Throwable t) {
          throw new IOException("ERROR while calling coprocessor",t);
        } 
        if(result == null)
          throw new IOException(retryErrMsg);
        else if(result.hasException())
          throw new IOException(result.getException());
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

    public synchronized void put(final TransactionState transactionState, final Put put, final boolean bool_addLocation) throws IOException {
      validatePut(put);
    	if (LOG.isTraceEnabled()) LOG.trace("TransactionalTable.put ENTRY");
    
      if (bool_addLocation) addLocation(transactionState, super.getRegionLocation(put.getRow()));
      final String regionName = super.getRegionLocation(put.getRow()).getRegionInfo().getRegionNameAsString();
      
      Batch.Call<TrxRegionService, PutTransactionalResponse> callable =
      new Batch.Call<TrxRegionService, PutTransactionalResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<PutTransactionalResponse> rpcCallback =
          new BlockingRpcCallback<PutTransactionalResponse>();
      @Override
      public PutTransactionalResponse call(TrxRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutTransactionalRequest.Builder builder = PutTransactionalRequest.newBuilder();
        builder.setTransactionId(transactionState.getTransactionId());
        builder.setRegionName(ByteString.copyFromUtf8(regionName));
  
        
        MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, put);
        builder.setPut(m1);
  
        instance.put(controller, builder.build(), rpcCallback);
        return rpcCallback.get();
      }
    };
    PutTransactionalResponse result = null; 
    try {
      int retryCount = 0;
      boolean retry = false;
      do {
        Iterator<Map.Entry<byte[], PutTransactionalResponse>> it= super.coprocessorService(TrxRegionService.class, 
                                                                                          put.getRow(), 
                                                                                          put.getRow(), 
                                                                                          callable)
                                                                                          .entrySet().iterator();
        if(it.hasNext()) {
          result = it.next().getValue();
          retry = false;
        }

        if(result == null || result.getException().contains("closing region")) {
          Thread.sleep(TransactionalTable.delay);
          retry = true;
          transactionState.setRetried(true);
          retryCount++;
        }

      } while(retryCount < TransactionalTable.retries && retry == true);
    } catch (Throwable e) {
      throw new IOException("ERROR while calling coprocessor", e);
    }    
    if(result == null)
      throw new IOException(retryErrMsg);
    else if(result.hasException())
      throw new IOException(result.getException());
    
    // put is void, may not need to check result
    if (LOG.isTraceEnabled()) LOG.trace("TransactionalTable.put EXIT");
  }

  public synchronized ResultScanner getScanner(final TransactionState transactionState, final Scan scan) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.getScanner");
    if (scan.getCaching() <= 0) {
        scan.setCaching(getScannerCaching());
    }
    
    Long value = (long) 0;
    TransactionalScanner scanner = new TransactionalScanner(this, transactionState, scan, value);
    
    return scanner;         
  }

  public boolean checkAndDelete(final TransactionState transactionState,
  		final byte[] row, final byte[] family, final byte[] qualifier, final byte[] value,
                     final Delete delete) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.checkAndDelete row: " + row + " family: " + family + " qualifier: " + qualifier + " value: " + value);
    if (!Bytes.equals(row, delete.getRow())) {
            throw new IOException("Action's getRow must match the passed row");
    }
    final String regionName = super.getRegionLocation(delete.getRow()).getRegionInfo().getRegionNameAsString();
    if(regionName == null) 
    	throw new IOException("Null regionName");
    Batch.Call<TrxRegionService, CheckAndDeleteResponse> callable =
      new Batch.Call<TrxRegionService, CheckAndDeleteResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<CheckAndDeleteResponse> rpcCallback =
        new BlockingRpcCallback<CheckAndDeleteResponse>();

      @Override
      public CheckAndDeleteResponse call(TrxRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndDeleteRequest.Builder builder = CheckAndDeleteRequest.newBuilder();
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

      CheckAndDeleteResponse result = null;
      try {
        int retryCount = 0;
        boolean retry = false;
        do {
          Iterator<Map.Entry<byte[], CheckAndDeleteResponse>> it = super.coprocessorService(TrxRegionService.class, 
                                                                                            delete.getRow(), 
                                                                                            delete.getRow(), 
                                                                                            callable)
                                                                                            .entrySet()
                                                                                            .iterator();
          if(it.hasNext()) {
            result = it.next().getValue();
            retry = false;
          }

          if(result == null || result.getException().contains("closing region")) {
            Thread.sleep(TransactionalTable.delay);
            retry = true;
            transactionState.setRetried(true);
            retryCount++;
          }
        } while (retryCount < TransactionalTable.retries && retry == true);
      } catch (Throwable e) {
        throw new IOException("ERROR while calling coprocessor",e);
      }
      if(result == null)
        throw new IOException(retryErrMsg);
      else if(result.hasException())
        throw new IOException(result.getException());
      return result.getResult();
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
		Batch.Call<TrxRegionService, CheckAndPutResponse> callable =
        new Batch.Call<TrxRegionService, CheckAndPutResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<CheckAndPutResponse> rpcCallback =
        new BlockingRpcCallback<CheckAndPutResponse>();

      @Override
      public CheckAndPutResponse call(TrxRegionService instance) throws IOException {
        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndPutRequest.Builder builder = CheckAndPutRequest.newBuilder();
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

      CheckAndPutResponse result = null;
      try {
        int retryCount = 0;
        boolean retry = false;
        do {
          Iterator<Map.Entry<byte[], CheckAndPutResponse>> it = super.coprocessorService(TrxRegionService.class, 
                                                                                      put.getRow(), 
                                                                                      put.getRow(), 
                                                                                      callable)
                                                                                      .entrySet()
                                                                                      .iterator();
          if(it.hasNext()) {
            result = it.next().getValue();
            retry = false;
          }

          if(result == null || result.getException().contains("closing region")) {
            Thread.sleep(TransactionalTable.delay);
            retry = true;
            transactionState.setRetried(true);
            retryCount++;
          }
        } while (retryCount < TransactionalTable.retries && retry == true);
      } catch (Throwable e) {        
        throw new IOException("ERROR while calling coprocessor ",e);       
      }

      if(result == null)
        throw new IOException(retryErrMsg);
      else if(result.hasException())
        throw new IOException(result.getException());

      return result.getResult();          
    }

       /**
   	 * Looking forward to TransactionalRegion-side implementation
   	 * 
   	 * @param transactionState
   	 * @param deletes
   	 * @throws IOException
   	 */
   	public void delete(final TransactionState transactionState,
   			List<Delete> deletes) throws IOException {
                        long transactionId = transactionState.getTransactionId();
		if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.delete[] <List> size: " + deletes.size() + ", transid: " + transactionId);
   		// collect all rows from same region
                        final Map<TransactionRegionLocation, List<Delete>> rows = new HashMap<TransactionRegionLocation, List<Delete>>();
                        HRegionLocation hlocation = null;
                        TransactionRegionLocation location = null;
   			List<Delete> list = null;
                        int size = 0;
   			for (Delete del : deletes) {
                                hlocation = this.getRegionLocation(del.getRow(), false);
                                location = new TransactionRegionLocation(hlocation.getRegionInfo(), hlocation.getServerName());
                if (LOG.isTraceEnabled()) LOG.trace("delete <List> with trRegion [" + location.getRegionInfo().getEncodedName() + "], endKey: "
                  + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " and transaction [" + transactionId + "], delete number: " + size);
			  if (!rows.containsKey(location)) {
                if (LOG.isTraceEnabled()) LOG.trace("delete adding new <List> for region [" + location.getRegionInfo().getRegionNameAsString() + "], endKey: "
                  + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " and transaction [" + transactionId + "], delete number: " + size);
   					list = new ArrayList<Delete>();
   					rows.put(location, list);
   				} else {
   					list = rows.get(location);
   				}
   				list.add(del);
                                size++;
   			}

   			final List<Delete> rowsInSameRegion = new ArrayList<Delete>();
                        for (Map.Entry<TransactionRegionLocation, List<Delete>> entry : rows.entrySet()) {
   				rowsInSameRegion.clear();
   				rowsInSameRegion.addAll(entry.getValue());
   				final String regionName = entry.getKey().getRegionInfo().getRegionNameAsString();
   				   				
   			 Batch.Call<TrxRegionService, DeleteMultipleTransactionalResponse> callable =
   	        new Batch.Call<TrxRegionService, DeleteMultipleTransactionalResponse>() {
   	      ServerRpcController controller = new ServerRpcController();
   	      BlockingRpcCallback<DeleteMultipleTransactionalResponse> rpcCallback =
   	        new BlockingRpcCallback<DeleteMultipleTransactionalResponse>();

   	      @Override
   	      public DeleteMultipleTransactionalResponse call(TrxRegionService instance) throws IOException {
   	        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteMultipleTransactionalRequest.Builder builder = DeleteMultipleTransactionalRequest.newBuilder();
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
   	    
   	   DeleteMultipleTransactionalResponse result = null;
 	      try {
 	        int retryCount = 0;
 	        boolean retry = false;
 	        do {
 	          Iterator<Map.Entry<byte[], DeleteMultipleTransactionalResponse>> it= super.coprocessorService(TrxRegionService.class, 
 	                                            entry.getValue().get(0).getRow(), 
 	                                            entry.getValue().get(0).getRow(), 
 	                                            callable)
 	                                            .entrySet().iterator();
 	          if(it.hasNext()) {
 	            result = it.next().getValue();
 	            retry = false;
 	          }

 	          if(result == null || result.getException().contains("closing region")) {
 	            Thread.sleep(TransactionalTable.delay);
 	            retry = true;
 	            transactionState.setRetried(true);
 	            retryCount++;
 	          }
 	        } while (retryCount < TransactionalTable.retries && retry == true);
 	        
 	      } catch (Throwable e) {
	        throw new IOException("ERROR while calling coprocessor", e);
 	      }

             if(result == null)
               throw new IOException(retryErrMsg);
             else if (result.hasException())
               throw new IOException(result.getException());
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
                long transactionId = transactionState.getTransactionId();
		if (LOG.isTraceEnabled()) LOG.trace("Enter TransactionalTable.put[] <List> size: " + puts.size() + ", transid: " + transactionId);
		// collect all rows from same region
		final Map<TransactionRegionLocation, List<Put>> rows = new HashMap<TransactionRegionLocation, List<Put>>();
		HRegionLocation hlocation = null;
                TransactionRegionLocation location = null;
		List<Put> list = null;
                int size = 0;
		for (Put put : puts) {
			validatePut(put);
			hlocation = this.getRegionLocation(put.getRow(), false);
                        location = new TransactionRegionLocation(hlocation.getRegionInfo(), hlocation.getServerName());
                if (LOG.isTraceEnabled()) LOG.trace("put <List> with trRegion [" + location.getRegionInfo().getEncodedName() + "], endKey: "
                  + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " and transaction [" + transactionId + "], put number: " + size);
			if (!rows.containsKey(location)) {
                if (LOG.isTraceEnabled()) LOG.trace("put adding new <List> for region [" + location.getRegionInfo().getRegionNameAsString() + "], endKey: "
                  + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " and transaction [" + transactionId + "], put number: " + size);
				list = new ArrayList<Put>();
				rows.put(location, list);
			} else {
				list = rows.get(location);
			}
			list.add(put);
                        size++;
		}

		final List<Put> rowsInSameRegion = new ArrayList<Put>();
		for (Map.Entry<TransactionRegionLocation, List<Put>> entry : rows.entrySet()) {
			rowsInSameRegion.clear();
			rowsInSameRegion.addAll(entry.getValue());
			final String regionName = entry.getKey().getRegionInfo().getRegionNameAsString();
			Batch.Call<TrxRegionService, PutMultipleTransactionalResponse> callable =
	        new Batch.Call<TrxRegionService, PutMultipleTransactionalResponse>() {
	      ServerRpcController controller = new ServerRpcController();
	      BlockingRpcCallback<PutMultipleTransactionalResponse> rpcCallback =
	        new BlockingRpcCallback<PutMultipleTransactionalResponse>();

	      @Override
	      public PutMultipleTransactionalResponse call(TrxRegionService instance) throws IOException {
	        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutMultipleTransactionalRequest.Builder builder = PutMultipleTransactionalRequest.newBuilder();
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
	    PutMultipleTransactionalResponse result = null;
      try {
        int retryCount = 0;
        boolean retry = false;
        do {
          Iterator<Map.Entry<byte[], PutMultipleTransactionalResponse>> it= super.coprocessorService(TrxRegionService.class, 
                                            entry.getValue().get(0).getRow(),
                                            entry.getValue().get(0).getRow(),                                             
                                            callable)
                                            .entrySet().iterator();
          if(it.hasNext()) {
            result = it.next().getValue();
            retry = false;
          }

          if(result == null || result.getException().contains("closing region")) {
            Thread.sleep(TransactionalTable.delay);
            retry = true;
            transactionState.setRetried(true);
            retryCount++;
          }
        } while (retryCount < TransactionalTable.retries && retry == true);
      } catch (Throwable e) {
        throw new IOException("ERROR while calling coprocessor",e);
      }
      if(result == null)
        throw new IOException(retryErrMsg);
      else if (result.hasException()) 
        throw new IOException(result.getException());
     }
		}
	
    // validate for well-formedness
    public void validatePut(final Put put) throws IllegalArgumentException {
        if (put.isEmpty()) {
            throw new IllegalArgumentException("No columns to insert");
        }
        if (maxKeyValueSize > 0) {
            for (List<Cell> list : put.getFamilyCellMap().values()) {
                for (Cell c : list) {
                    if (KeyValueUtil.length(c) > maxKeyValueSize) {
                        throw new IllegalArgumentException("KeyValue size too large");
                    }
                }
            }
        }
    }

    private int maxKeyValueSize;

    public HRegionLocation getRegionLocation(byte[] row, boolean f)
                                  throws IOException {
        return super.getRegionLocation(row,f);
    }
    public void close()  throws IOException { super.close(); }
        public void setAutoFlush(boolean autoFlush, boolean b)
    {
        super.setAutoFlush(autoFlush, b);
    }
    public org.apache.hadoop.conf.Configuration getConfiguration()
    {
        return super.getConfiguration();
    }
    public void flushCommits()
                  throws IOException {
         super.flushCommits();
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
    public ResultScanner getScanner(Scan scan, float DOPparallelScanner) throws IOException
    {
        if (scan.isSmall() || DOPparallelScanner == 0)
            return super.getScanner(scan);
        else
            return new TrafParallelClientScanner(this.connection, scan, getName(), DOPparallelScanner);       
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
    public void put(Put p) throws IOException
    {
        super.put(p);
    }
    public void put(List<Put> p) throws IOException
    {
        super.put(p);
    }
    public boolean checkAndDelete(byte[] row, byte[] family, byte[] qualifier, byte[] value,  Delete delete) throws IOException
    {
        return super.checkAndDelete(row,family,qualifier,value,delete);
    }
	
}

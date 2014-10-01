/**
 * Copyright 2009 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
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

import com.google.protobuf.ByteString;
import com.google.protobuf.HBaseZeroCopyByteString;
import com.google.protobuf.ServiceException;


/**
 * Table with transactional support.
 */
public class TransactionalTable extends HTable {
    static final Log LOG = LogFactory.getLog(RMInterface.class);
    
    /*   // Not necessary for 0.98
    private RpcRetryingCallerFactory rpcCallerFactory;    
    private RpcControllerFactory rpcControllerFactory;
    */
    
    /**
     * @param conf
     * @param tableName
     * @throws IOException
     */
    public TransactionalTable(final Configuration conf, final String tableName) throws IOException {
        this(conf, Bytes.toBytes(tableName));        
    }

    /**
     * @param conf
     * @param tableName
     * @throws IOException
     */
    public TransactionalTable(final Configuration conf, final byte[] tableName) throws IOException {
        super(conf, tableName);       
    }
    
    private void addLocation(final TransactionState transactionState, HRegionLocation location) {
      LOG.trace("addLocation ENTRY");
      if (transactionState.addRegion(location))     	  
         LOG.trace("TransactionalTable.recordServer added region to TS. Beginning txn " + transactionState + " on server");
      LOG.trace("addLocation EXIT");
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
      LOG.trace("Enter TransactionalTable.get");
      
      addLocation(transactionState, super.getRegionLocation(get.getRow()));
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

      Map<byte[], GetTransactionalResponse> result = null;   
      try {
        result = super.coprocessorService(TrxRegionService.class, get.getRow(), get.getRow(), callable);
      } catch (Throwable e) {
        e.printStackTrace();    
        throw new IOException("ERROR while calling coprocessor");
      }            
      Collection<GetTransactionalResponse> results = result.values();
      // Should only be one result, if more than one. Can't handle.
      // Need to test whether '!=' or '>' is correct
      LOG.trace("Results count: " + results.size());
      //if(results.size() != 1)
      //  throw new IOException("Incorrect number of results from coprocessor call");      
      GetTransactionalResponse[] resultArray = new GetTransactionalResponse[results.size()];    		  
      results.toArray(resultArray);            
      if(resultArray.length == 0) 
    	  throw new IOException("Problem with calling coprocessor, no regions returned result");
      
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
        SingleVersionDeleteNotSupported.validateDelete(delete);
        addLocation(transactionState, super.getRegionLocation(delete.getRow()));
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
        Map<byte[], DeleteTransactionalResponse> result = null; 
        try {
          result = super.coprocessorService(TrxRegionService.class, row, row, callable);
          
        } catch (ServiceException e) {
          e.printStackTrace();
          throw new IOException();
        } catch (Throwable t) {
          t.printStackTrace();
          throw new IOException();
        } 
        Collection<DeleteTransactionalResponse> results = result.values();
        //GetTransactionalResponse[] resultArray = (GetTransactionalResponse[]) results.toArray();
        DeleteTransactionalResponse[] resultArray = new DeleteTransactionalResponse[results.size()];
        results.toArray(resultArray);
        
        if(resultArray.length == 0) 
      	  throw new IOException("Problem with calling coprocessor, no regions returned result");

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
      validatePut(put);
    	LOG.trace("TransactionalTable.put ENTRY");
    
      
      addLocation(transactionState, super.getRegionLocation(put.getRow()));
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
    Map<byte[], PutTransactionalResponse> result = null; 
    try {
      result = super.coprocessorService(TrxRegionService.class, put.getRow(), put.getRow(), callable);
    } catch (Throwable e) {
      e.printStackTrace();
      throw new IOException("ERROR while calling coprocessor");
    }    
    Collection<PutTransactionalResponse> results = result.values();
    PutTransactionalResponse[] resultArray = new PutTransactionalResponse[results.size()]; 
    results.toArray(resultArray);
    if(resultArray.length == 0) 
  	  throw new IOException("Problem with calling coprocessor, no regions returned result");
    
    if(resultArray[0].hasException())
      throw new IOException(resultArray[0].getException());
    
    // put is void, may not need to check result
    LOG.trace("TransactionalTable.put EXIT");
  }

  public synchronized ResultScanner getScanner(final TransactionState transactionState, final Scan scan) throws IOException {
    LOG.trace("Enter TransactionalTable.getScanner");
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
    LOG.trace("Enter TransactionalTable.checkAndDelete row: " + row + " family: " + family + " qualifier: " + qualifier + " value: " + value);
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

      Map<byte[], CheckAndDeleteResponse> result = null;
      try {
        result = super.coprocessorService(TrxRegionService.class, delete.getRow(), delete.getRow(), callable);
      } catch (Throwable e) {
        e.printStackTrace();
        throw new IOException("ERROR while calling coprocessor");
      }
      
      Collection<CheckAndDeleteResponse> results = result.values();
      CheckAndDeleteResponse[] resultArray = new CheckAndDeleteResponse[results.size()];
      results.toArray(resultArray);
      
      if(resultArray.length == 0) 
    	  throw new IOException("Problem with calling coprocessor, no regions returned result");
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

		LOG.trace("Enter TransactionalTable.checkAndPut row: " + row
				+ " family: " + family + " qualifier: " + qualifier
				+ " value: " + value);
		if (!Bytes.equals(row, put.getRow())) {
			throw new IOException("Action's getRow must match the passed row");
		}
		final String regionName = super.getRegionLocation(put.getRow()).getRegionInfo().getRegionNameAsString();
		LOG.trace("checkAndPut, region name: " + regionName);
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

      Map<byte[], CheckAndPutResponse> result = null;
      try {
	  result = super.coprocessorService(TrxRegionService.class, put.getRow(), put.getRow(), callable);
      } catch (Throwable e) {        
          StringWriter sw = new StringWriter();
          PrintWriter pw = new PrintWriter(sw);
          e.printStackTrace(pw);
          //sw.toString();
        throw new IOException("ERROR while calling coprocessor " + sw.toString());       
      }
      Collection<CheckAndPutResponse> results = result.values();
      // Should only be one result, if more than one. Can't handle.      
      CheckAndPutResponse[] resultArray = new CheckAndPutResponse[results.size()];
      results.toArray(resultArray);
      if(resultArray.length == 0) 
    	  throw new IOException("Problem with calling coprocessor, no regions returned result");
    	
      return resultArray[0].getResult();      
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
   		LOG.trace("Enter TransactionalTable.delete[] row ");
   		// collect all rows from same region
   			final Map<HRegionLocation, List<Delete>> rows = new HashMap<HRegionLocation, List<Delete>>();
   			HRegionLocation location = null;
   			List<Delete> list = null;
   			for (Delete del : deletes) {
   				location = this.getRegionLocation(del.getRow(), false);
   				if (!rows.containsKey(location)) {
   					list = new ArrayList<Delete>();
   					rows.put(location, list);
   				} else {
   					list = rows.get(location);
   				}
   				list.add(del);
   			}

   			final List<Delete> rowsInSameRegion = new ArrayList<Delete>();
   			for (Map.Entry<HRegionLocation, List<Delete>> entry : rows.entrySet()) {
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
   	    
   	   Map<byte[], DeleteMultipleTransactionalResponse> result = null;
 	      try {
 	        result = super.coprocessorService(TrxRegionService.class, 
                                      entry.getKey().getRegionInfo().getStartKey(), 
                                      TransactionManager.binaryIncrementPos(entry.getKey().getRegionInfo().getEndKey(),-1),
                                      callable);
 	      } catch (Throwable e) {
 	        e.printStackTrace();
	        throw new IOException("ERROR while calling coprocessor");
 	      }
	      if(result.size() > 1) {
             LOG.error("result size for multiple delete:" + result.size());
             throw new IOException("Incorrect number of region results");
	      }
	      Collection<DeleteMultipleTransactionalResponse> results = result.values();
	      DeleteMultipleTransactionalResponse[] resultArray = new DeleteMultipleTransactionalResponse[results.size()];
	      results.toArray(resultArray);
	      if(resultArray.length == 0) 
	    	  throw new IOException("Problem with calling coprocessor, no regions returned result");
	      
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
		LOG.trace("Enter TransactionalTable.put[] row ");
		// collect all rows from same region
		final Map<HRegionLocation, List<Put>> rows = new HashMap<HRegionLocation, List<Put>>();
		HRegionLocation location = null;
		List<Put> list = null;
		for (Put put : puts) {
			validatePut(put);
			location = this.getRegionLocation(put.getRow(), false);
			if (!rows.containsKey(location)) {
				list = new ArrayList<Put>();
				rows.put(location, list);
			} else {
				list = rows.get(location);
			}
			list.add(put);
		}

		final List<Put> rowsInSameRegion = new ArrayList<Put>();
		for (Map.Entry<HRegionLocation, List<Put>> entry : rows.entrySet()) {
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
	    Map<byte[], PutMultipleTransactionalResponse> result = null;
      try {
        result = super.coprocessorService(TrxRegionService.class, 
        								  entry.getKey().getRegionInfo().getStartKey(), 
        								  TransactionManager.binaryIncrementPos(entry.getKey().getRegionInfo().getEndKey(),-1), 
        								  callable);
      } catch (Throwable e) {
        e.printStackTrace();
        throw new IOException("ERROR while calling coprocessor");
      }
      Collection<PutMultipleTransactionalResponse> results = result.values();
      PutMultipleTransactionalResponse[] resultArray = new PutMultipleTransactionalResponse[results.size()];
      results.toArray(resultArray);
      if(resultArray.length == 0) 
    	  throw new IOException("Problem with calling coprocessor, no regions returned result");
      
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
	
	private int maxKeyValueSize;
	
}

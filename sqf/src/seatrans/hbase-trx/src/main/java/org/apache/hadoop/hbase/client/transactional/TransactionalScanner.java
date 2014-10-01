package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.util.LinkedList;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.client.AbstractClientScanner;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.util.Bytes;

import com.google.protobuf.ByteString;


/*
 *   Simple Transaction Scanner, doesn't have full functionality ie caching
 *                               also does not avoid splitting, that will require an extra call
 *                               such as openScanner to be run on all regions and the region names 
 *                               along with the scanner ID's saved 
 */
public class TransactionalScanner extends AbstractClientScanner {
	private final Log LOG = LogFactory.getLog(this.getClass());
	public Scan scan;
	public Long scannerID;
	public TransactionState ts;
	public TransactionalTable ttable;
	protected boolean closed = false;
        protected boolean doNotCloseOnLast = true;
        protected int nbRows = 100;
	private boolean firstEntry = true;
	public HRegionInfo currentRegion;
	//public String currentRegionName;
	public byte[] currentBeginKey;
	public byte[] currentEndKey;
	protected final LinkedList<Result> cache = new LinkedList<Result>();
	
	
	public TransactionalScanner(final TransactionalTable ttable, final TransactionState ts, final Scan scan, final Long scannerID) {
		super();
		this.scan = scan;
		this.scannerID = scannerID;
		this.ts = ts;
		this.ttable = ttable;
                this.nbRows = scan.getCaching();
                if (nbRows <= 0)
                  nbRows = 100;
		try {
			nextScanner(false);
		}catch (IOException e) {
			LOG.error("nextScanner error");
		}
	}

    // returns true if the passed region endKey
    protected boolean checkScanStopRow(final byte [] endKey) {
      if (this.scan.getStopRow().length > 0) {
        // there is a stop row, check to see if we are past it.
        byte [] stopRow = scan.getStopRow();
        int cmp = Bytes.compareTo(stopRow, 0, stopRow.length,
          endKey, 0, endKey.length);
        if (cmp <= 0) {
          // stopRow <= endKey (endKey is equals to or larger than stopRow)
          // This is a stop.
          return true;
        }
      }
      return false; //unlikely.
    }
    
	@Override
	public void close() {
		// Make call to closeScanner(), close all regions at once for now
		// TODO: Still needs work.  Should close a particular scanner in nextScanner()
		LOG.trace("close() -- ENTRY txID: " + ts.getTransactionId());
		if(closed == true) {
			LOG.trace("close()  already closed -- EXIT txID: " + ts.getTransactionId());
			return;
		}
		 Batch.Call<TrxRegionService, CloseScannerResponse> callable =
			        new Batch.Call<TrxRegionService, CloseScannerResponse>() {
			      ServerRpcController controller = new ServerRpcController();
			      BlockingRpcCallback<CloseScannerResponse> rpcCallback =
			        new BlockingRpcCallback<CloseScannerResponse>();

			      @Override
			      public CloseScannerResponse call(TrxRegionService instance) throws IOException {
			        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerRequest.Builder builder = CloseScannerRequest.newBuilder();
			        builder.setTransactionId(ts.getTransactionId());
			        builder.setScannerId(scannerID);
			        
			        // TODO: put region name at some point
			        builder.setRegionName(ByteString.copyFromUtf8(""));

			        instance.closeScanner(controller, builder.build(), rpcCallback);
			        return rpcCallback.get();
			      }
			    };

			    Map<byte[], CloseScannerResponse> result = null;

			    try {
			      result = ttable.coprocessorService(TrxRegionService.class, 
                                                     currentBeginKey, 
                                                     currentEndKey,
                                                     callable);
			    } catch (Throwable e) {
			      e.printStackTrace();
			    }

			      for (CloseScannerResponse cresponse : result.values())
			      {
			        boolean hasException = cresponse.getHasException();
			        String exception = cresponse.getException();
			        if (hasException)
			          LOG.trace("  CloseScannerResponse exception " + exception );
			      }
	    this.closed = true;
	    LOG.trace("close() -- EXIT txID: " + ts.getTransactionId());
		
	}
	
	protected boolean nextScanner(final boolean done) throws IOException{
		LOG.trace("nextScanner() -- ENTRY txID: " + ts.getTransactionId());
		
		//byte [] localStartKey;
		
		if(this.currentBeginKey != null) {
			LOG.trace("nextScanner() currentBeginKey += null txID: " + ts.getTransactionId());
                        if (doNotCloseOnLast)
			  close();
			byte [] endKey = this.currentRegion.getEndKey();

			if(endKey == null ||
			   Bytes.equals(endKey, HConstants.EMPTY_BYTE_ARRAY) ||
			   checkScanStopRow(endKey) ||
			   done) {
				
				LOG.trace("nextScanner() -- EXIT -- returning false txID: " + ts.getTransactionId());
				return false;				
			}
			//localStartKey = endKey;
			this.currentBeginKey = TransactionManager.binaryIncrementPos(endKey,1);
			
		}
		else {
			// currentBeginKey is NULL so this is first entry
			//localStartKey = this.scan.getStartRow();
			this.currentBeginKey = this.scan.getStartRow();					
		}
		
		this.currentRegion = ttable.getRegionLocation(this.currentBeginKey).getRegionInfo();
		LOG.trace("Region Info: " + currentRegion.getRegionNameAsString());
		this.currentEndKey = TransactionManager.binaryIncrementPos(currentRegion.getEndKey(), -1);
		//endKey = TransactionManager.binaryIncrementPos(endKey_orig, -1);
		// This is where we open and get the scanner with the startKey, endKey
		
		this.closed = false;
	   Batch.Call<TrxRegionService, OpenScannerResponse> callable =
		        new Batch.Call<TrxRegionService, OpenScannerResponse>() {
		      ServerRpcController controller = new ServerRpcController();
		      BlockingRpcCallback<OpenScannerResponse> rpcCallback =
		        new BlockingRpcCallback<OpenScannerResponse>();

		      @Override
		      public OpenScannerResponse call(TrxRegionService instance) throws IOException {
		        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerRequest.Builder builder = OpenScannerRequest.newBuilder();
		        builder.setTransactionId(ts.getTransactionId());
		        builder.setRegionName(ByteString.copyFromUtf8(currentRegion.getRegionNameAsString()));

		        //Scan scan = new Scan();
		        //scan.addColumn(FAMILY, Bytes.toBytes(1));

		        builder.setScan(ProtobufUtil.toScan(scan));

		        instance.openScanner(controller, builder.build(), rpcCallback);
		        return rpcCallback.get();
		      }
		    };

		    Map<byte[], OpenScannerResponse> result = null;

		    try {
		      result = ttable.coprocessorService(TrxRegionService.class, 
		    		  							 this.currentBeginKey,
		    		  							 this.currentEndKey,
		    		  							 //this.currentEndKey, 
		    		  							 callable);
		      		      
		    } catch (Throwable e) {
		      e.printStackTrace();
		    }

		      for (OpenScannerResponse oresponse : result.values())
		      {
		        
		        String exception = oresponse.getException();
		        boolean hasException = oresponse.getHasException();
		        if (hasException) {
		          //LOG.error("  OpenScannerResponse exception " + exception );
		          // Exception is ok for now
		          LOG.trace("nextScanner() -- EXIT -- encountered EXCEPTION returning false txID: " + ts.getTransactionId());		          		          
		        }
		        else{
		          
		        	this.scannerID = oresponse.getScannerId();
		        	LOG.trace("  OpenScannerResponse scannerId is " + this.scannerID );
		        }
		      }

		LOG.trace("nextScanner() -- EXIT -- returning true txID: " + ts.getTransactionId());
		return true;
	}

	@Override
	public Result next() throws IOException {
		// Commenting out ENTRY/EXIT lines since they fill up logs, uncomment for
		//    debugging purposes
		//LOG.trace("next() -- ENTRY txID: " + ts.getTransactionId());
		
		if(cache.size() == 0 && firstEntry == true) { 
			firstEntry = false;
			do { 
				LOG.trace("next() before coprocessor PerformScan call txID: " + ts.getTransactionId());		
				Batch.Call<TrxRegionService, PerformScanResponse> callable = 
				        new Batch.Call<TrxRegionService, PerformScanResponse>() {
				      ServerRpcController controller = new ServerRpcController();
				      BlockingRpcCallback<PerformScanResponse> rpcCallback = 
				        new BlockingRpcCallback<PerformScanResponse>();         

				      @Override
				      public PerformScanResponse call(TrxRegionService instance) throws IOException {        
				        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanRequest.Builder builder = PerformScanRequest.newBuilder();        
				        builder.setTransactionId(ts.getTransactionId());
				        builder.setRegionName(ByteString.copyFromUtf8(currentRegion.getRegionNameAsString()));
				        builder.setScannerId(scannerID);
				        builder.setNumberOfRows(nbRows);
                                        if (doNotCloseOnLast)
				          builder.setCloseScanner(false);
                                        else
				          builder.setCloseScanner(true);
				        builder.setNextCallSeq(0);

				        instance.performScan(controller, builder.build(), rpcCallback);
				        return rpcCallback.get();        
				      }
				    };
				 
				    Map<byte[], PerformScanResponse> presult = null;   
				    org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result[]
				    results = null;


				    try {
				      presult = ttable.coprocessorService(TrxRegionService.class, currentBeginKey, currentEndKey, callable);
				    } catch (Throwable e) {
				    	LOG.error("ERROR when calling PerformScan coprocessor" + e.toString());
				    	//e.printStackTrace();     
				    }

				      int count = 0;
				      boolean hasMore = false;

				      org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result
				        result = null;
				            
				      for (PerformScanResponse presponse : presult.values())
				      {
				        if (presponse.getHasException())
				        {
				          String exception = presponse.getException();
				          LOG.trace("  PerformScanResponse exception " + exception );
				        }
				        else
				        {
				          count = presponse.getResultCount();
				          results = 
				            new org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result[count];

				          for (int i = 0; i < count; i++)
				          {
				            result = presponse.getResult(i);
				            if (result != null) {
				            	cache.add(ProtobufUtil.toResult(result));
				            }
				            hasMore = presponse.getHasMore();
				            results[i] = result;
				            result = null;
				            LOG.trace("  PerformScan response count " + count + ", hasMore is " + hasMore + ", result " + results[i] );
				          }
				        }
				      }
				      
				      if(!hasMore) {
				    	  LOG.trace("hasMore is false");
				    	  if(nextScanner(false) == true){
				    		  LOG.trace("nextScanner == true, continuing");
				    		  continue;
				    	  }
				    	  else {
                              LOG.trace("nextScanner == false, break");
				    		  break;
				    	  }
				    		  
				      }  
			}while (true); 

		}
		
		if (cache.size() > 0)  {
			//LOG.trace("next() -- EXIT txID: " + ts.getTransactionId());
			return cache.poll();
		}

		
		// Exhausted scanner before calling close, then return null
		//LOG.trace("next() -- EXIT -- returning null txID: " + ts.getTransactionId());
		return null;
	}


}

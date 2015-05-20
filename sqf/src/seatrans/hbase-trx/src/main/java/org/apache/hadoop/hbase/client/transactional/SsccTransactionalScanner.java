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
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRegionService;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerResponse;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.CoprocessorRpcChannel;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.util.Bytes;

import com.google.protobuf.ByteString;


/*
 *   Simple Transaction Scanner
 */
public class SsccTransactionalScanner extends AbstractClientScanner {
    private final Log LOG = LogFactory.getLog(this.getClass());
    public Scan scan;
    public Long scannerID;
    public TransactionState ts;
    public SsccTransactionalTable ttable;
    protected boolean closed = false;
    // Experiment with this parameter, may be faster without having to send the final close()
    protected boolean doNotCloseOnLast = true;
    protected int nbRows = 100;
    protected long nextCallSeq = 0;
    private boolean hasMore = true;
    private boolean moreScanners = true;
    public HRegionInfo currentRegion;
    public byte[] currentBeginKey;
    public byte[] currentEndKey;
    protected final LinkedList<Result> cache = new LinkedList<Result>();

    public SsccTransactionalScanner(final SsccTransactionalTable ttable, final TransactionState ts, final Scan scan, final Long scannerID) {
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

    protected boolean checkScanStopRow(final byte [] endKey) {
      if (this.scan.getStopRow().length > 0) {
        byte [] stopRow = scan.getStopRow();
        int cmp = Bytes.compareTo(stopRow, 0, stopRow.length,
          endKey, 0, endKey.length);
        if (cmp <= 0) {
          return true;
        }
      }
      return false;
    }
    
    @Override
    public void close() {
        if(LOG.isTraceEnabled()) LOG.trace("close() -- ENTRY txID: " + ts.getTransactionId());
        if(closed) {
            if(LOG.isTraceEnabled()) LOG.trace("close()  already closed -- EXIT txID: " + ts.getTransactionId());
            return;
        }
        SsccRegionProtos.SsccCloseScannerRequest.Builder requestBuilder = SsccCloseScannerRequest.newBuilder();
        requestBuilder.setTransactionId(ts.getTransactionId());
        requestBuilder.setRegionName(ByteString.copyFromUtf8(currentRegion.getRegionNameAsString()));
        requestBuilder.setScannerId(scannerID);
        SsccRegionProtos.SsccCloseScannerRequest closeRequest = requestBuilder.build();
        try {
            CoprocessorRpcChannel channel = ttable.coprocessorService(this.currentBeginKey);
            SsccRegionService.BlockingInterface trxService = SsccRegionService.newBlockingStub(channel);
            SsccRegionProtos.SsccCloseScannerResponse response = trxService.closeScanner(null, closeRequest);
            String exception = response.getException();
            if(response.getHasException()) {
                String errMsg = "closeScanner encountered Exception txID: " +
                        ts.getTransactionId() + " Exception: " + exception;
                    LOG.error(errMsg);
            }
        }
        catch (Throwable e) {
            String errMsg = "CloseScanner error on coprocessor call, scannerID: " + this.scannerID + " " + e;
            LOG.error(errMsg);
        }

        this.closed = true;
        if(LOG.isTraceEnabled()) LOG.trace("close() -- EXIT txID: " + ts.getTransactionId());
    }

    protected boolean nextScanner(final boolean done) throws IOException{
        if(LOG.isTraceEnabled()) LOG.trace("nextScanner() -- ENTRY txID: " + ts.getTransactionId());
        if(this.currentBeginKey != null) {
            if(LOG.isTraceEnabled()) LOG.trace("nextScanner() currentBeginKey != null txID: " + ts.getTransactionId());
            if (doNotCloseOnLast)
              close();
            if((this.currentEndKey == HConstants.EMPTY_END_ROW) || 
                Bytes.equals(this.currentEndKey, HConstants.EMPTY_BYTE_ARRAY) ||
                checkScanStopRow(this.currentEndKey) || 
                done) {
                LOG.trace("endKey: " + Bytes.toString(this.currentEndKey));
                if(LOG.isTraceEnabled()) LOG.trace("nextScanner() -- EXIT -- returning false txID: " + ts.getTransactionId());
                this.moreScanners = false;
                return false;
            }
            else
                this.currentBeginKey = TransactionManager.binaryIncrementPos(this.currentEndKey,1);
        }
        else {
            // First call to nextScanner
            this.currentBeginKey = this.scan.getStartRow();
        }

        this.currentRegion = ttable.getRegionLocation(this.currentBeginKey).getRegionInfo();
        this.currentEndKey = this.currentRegion.getEndKey();

        if(LOG.isTraceEnabled()) LOG.trace("Region Info: " + currentRegion.getRegionNameAsString());
        if(this.currentEndKey != HConstants.EMPTY_END_ROW)
            this.currentEndKey = TransactionManager.binaryIncrementPos(currentRegion.getEndKey(), -1);

        this.closed = false;

      SsccRegionProtos.SsccOpenScannerRequest.Builder requestBuilder = SsccOpenScannerRequest.newBuilder();
      requestBuilder.setTransactionId(ts.getTransactionId());
      requestBuilder.setStartId(ts.getStartId());
      requestBuilder.setRegionName(ByteString.copyFromUtf8(currentRegion.getRegionNameAsString()));
      requestBuilder.setScan(ProtobufUtil.toScan(scan));
      SsccRegionProtos.SsccOpenScannerRequest openRequest = requestBuilder.build();
      try {
          CoprocessorRpcChannel channel = ttable.coprocessorService(this.currentBeginKey);
          SsccRegionService.BlockingInterface trxService = SsccRegionService.newBlockingStub(channel);
          SsccRegionProtos.SsccOpenScannerResponse response = trxService.openScanner(null, openRequest);
          String exception = response.getException();
          if(response.getHasException()) {
              String errMsg = "nextScanner encountered Exception txID: " +
                      ts.getTransactionId() + " Exception: " + exception;
                  LOG.error(errMsg);
                  throw new IOException(errMsg);
          }
          this.scannerID = response.getScannerId();
      }
      catch (Throwable e) {
          String errMsg = "OpenScanner error on coprocessor call, scannerID: " + this.scannerID;
          LOG.error(errMsg);
          throw new IOException(errMsg);
      }
        this.nextCallSeq = 0;
        if(LOG.isTraceEnabled()) LOG.trace("nextScanner() -- EXIT -- returning true txID: " + ts.getTransactionId());
        return true;
    }

    @Override
    public Result next() throws IOException {
        // if (LOG.isTraceEnabled()) LOG.trace("next -- ENTRY txID: " + ts.getTransactionId() + " cache size: " + cache.size());
        if(cache.size() == 0) {
            if (LOG.isTraceEnabled()) LOG.trace("next -- cache.size() == 0 txID: " + ts.getTransactionId());
            if(this.hasMore) {
                if (LOG.isTraceEnabled())
                    LOG.trace("next() before coprocessor PerformScan call txID: " + ts.getTransactionId());
                final long nextCallSeqInput = this.nextCallSeq;
                SsccRegionProtos.SsccPerformScanRequest.Builder requestBuilder = SsccPerformScanRequest.newBuilder();
                requestBuilder.setTransactionId(ts.getTransactionId());
                requestBuilder.setStartId(ts.getStartId());
                requestBuilder.setRegionName(ByteString.copyFromUtf8(currentRegion.getRegionNameAsString()));
                requestBuilder.setScannerId(scannerID);
                requestBuilder.setNumberOfRows(nbRows);
                if (doNotCloseOnLast)
                    requestBuilder.setCloseScanner(false);
                else
                    requestBuilder.setCloseScanner(true);
                requestBuilder.setNextCallSeq(nextCallSeqInput);
                SsccRegionProtos.SsccPerformScanRequest perfScanRequest = requestBuilder.build();
                SsccRegionProtos.SsccPerformScanResponse response;
                try {
                    CoprocessorRpcChannel channel = ttable.coprocessorService(this.currentBeginKey);
                    SsccRegionService.BlockingInterface trxService = SsccRegionService.newBlockingStub(channel);
                    response = trxService.performScan(null, perfScanRequest);
                    String exception = response.getException();
                    if(response.getHasException()) {
                        String errMsg = "peformScan encountered Exception txID: " +
                                ts.getTransactionId() + " Exception: " + exception;
                            LOG.error(errMsg);
                            throw new IOException(errMsg);
                    }
                }
                catch (Throwable e) {
                    String errMsg = "PerformScan error on coprocessor call, scannerID: " + this.scannerID;
                    LOG.error(errMsg);
                    throw new IOException(errMsg);
                }
                int count;
                org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result result;

                this.nextCallSeq = response.getNextCallSeq();
                count = response.getResultCount();
                if (LOG.isTraceEnabled()) LOG.trace("next() nextCallSeq: " + this.nextCallSeq +
                        " count: " + count);
                if(count == 0) {
                    this.hasMore = false;
                }
                else {
                    for (int i = 0; i < count; i++) {
                        result = response.getResult(i);
                        if (result != null) {
                            cache.add(ProtobufUtil.toResult(result));
                        }
                        this.hasMore = response.getHasMore();
                        if (LOG.isTraceEnabled())
                            LOG.trace("  PerformScan response count " + count + ", hasMore is " + hasMore + ", result " + result);
                    }

                }
            }
            else {
              if(LOG.isTraceEnabled()) LOG.trace("hasMore is false");
              if(nextScanner(false)){
                  if(LOG.isTraceEnabled()) LOG.trace("next(), nextScanner == true nextCallSeq: " + this.nextCallSeq);
                  this.hasMore = true;
                  return next();
              }
              else {
                  if(LOG.isTraceEnabled()) LOG.trace("next(), nextScanner == false");
                  this.moreScanners = false;
                  return null;
              }
            }
        }
        if (cache.size() > 0)  {
            // if(LOG.isTraceEnabled()) LOG.trace("next() returning cache.poll()");
            return cache.poll();
        }
        else if(this.moreScanners){
            if(LOG.isTraceEnabled()) LOG.trace("next() more scanners");
            return next();
        }
        else {
            if(LOG.isTraceEnabled()) LOG.trace("next() returning null");
            return null;
        }
    }
}

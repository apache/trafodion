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

package org.trafodion.sql;

// This class implements an efficient mechanism to read hdfs files
// Trafodion ExHdfsScan operator provides a range of scans to be performed.
// The range consists of a hdfs filename, offset and length to be read
// This class takes in two ByteBuffers. These ByteBuffer can be either direct buffers
// backed up native buffers or indirect buffer backed by java arrays.
// All the ranges are read alternating between the two buffers using ExecutorService
// using CachedThreadPool mechanism. 
// For a given HdfsScan instance, only one thread(IO thread) is scheduled to read
// the next full or partial buffer while the main thread processes the previously
// read information from the other buffer

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.conf.Configuration;
import java.nio.ByteBuffer;
import java.io.IOException;
import java.util.concurrent.Callable;
import java.util.concurrent.Future;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ExecutionException;
import org.trafodion.sql.HDFSClient;

import org.apache.hadoop.hive.metastore.HiveMetaStoreClient;
import org.apache.hadoop.hive.conf.HiveConf;
import org.apache.hadoop.hive.metastore.api.Table;
import org.apache.hadoop.hive.metastore.api.StorageDescriptor;
import org.apache.hadoop.fs.FileStatus;
import java.net.URI;

public class HdfsScan 
{
   static Logger logger_ = Logger.getLogger(HdfsScan.class.getName());
   private ByteBuffer buf_[];
   private int bufLen_[];
   private HDFSClient hdfsClient_[];
   private int currRange_;
   private long currPos_;
   private long lenRemain_;
   private int lastBufCompleted_ = -1;
   private boolean scanCompleted_;
   
   class HdfsScanRange 
   {
      String filename_;
      long pos_;
      long len_;
      int tdbRangeNum_;
      
      HdfsScanRange(String filename, long pos, long len, int tdbRangeNum)
      {
         filename_ = filename;
         pos_ = pos;
         len_ = len;
         tdbRangeNum_ = tdbRangeNum;
      }
   }
   
   private HdfsScanRange hdfsScanRanges_[];
    
   static {
      String confFile = System.getProperty("trafodion.log4j.configFile");
      System.setProperty("trafodion.root", System.getenv("TRAF_HOME"));
   }

   public HdfsScan() 
   {
   }

   public void setScanRanges(ByteBuffer buf1, ByteBuffer buf2, String filename[], long pos[], long len[], int rangeNum[]) throws IOException
   {
      buf_ = new ByteBuffer[2];
      bufLen_ = new int[2];

      buf_[0] = buf1;
      buf_[1] = buf2;

      for (int i = 0; i < 2 ; i++) {
          if (buf_[i].hasArray())
             bufLen_[i] = buf_[i].array().length;
          else
             bufLen_[i] = buf_[i].capacity();
      }
      hdfsClient_ = new HDFSClient[2];
      hdfsScanRanges_ = new HdfsScanRange[filename.length]; 
      for (int i = 0; i < filename.length; i++) {
         hdfsScanRanges_[i] = new HdfsScanRange(filename[i], pos[i], len[i], rangeNum[i]);
      }
      if (hdfsScanRanges_.length > 0) {
         currRange_ = 0;
         currPos_ = hdfsScanRanges_[currRange_].pos_;
         lenRemain_ = hdfsScanRanges_[currRange_].len_; 
         hdfsScanRange(0, 0);
      }
      scanCompleted_ = false;
   }

   public void hdfsScanRange(int bufNo, int bytesCompleted) throws IOException
   {
      lenRemain_ -= bytesCompleted;
      currPos_ += bytesCompleted; 
      int readLength;
      if (lenRemain_ <= 0) {
         if (currRange_  == (hdfsScanRanges_.length-1)) {
            scanCompleted_ = true;
            return;
         }
         else {
            currRange_++;
            currPos_ = hdfsScanRanges_[currRange_].pos_;
            lenRemain_ = hdfsScanRanges_[currRange_].len_; 
         }
      } 
      if (lenRemain_ > bufLen_[bufNo])
         readLength = bufLen_[bufNo];
      else
         readLength = (int)lenRemain_;
      if (! scanCompleted_) {
         if (logger_.isDebugEnabled())
            logger_.debug(" CurrentRange " + hdfsScanRanges_[currRange_].tdbRangeNum_ + " LenRemain " + lenRemain_ + " BufNo " + bufNo); 
         hdfsClient_[bufNo] = new HDFSClient(bufNo, hdfsScanRanges_[currRange_].tdbRangeNum_, hdfsScanRanges_[currRange_].filename_, buf_[bufNo], currPos_, readLength);
      }
   } 
   
   public int[] trafHdfsRead() throws IOException, InterruptedException, ExecutionException
   {
      int[] retArray;
      int bytesRead;
      int bufNo;
      int rangeNo;
      int isEOF;
  
      if (hdfsScanRanges_ == null)
         throw new IOException("Scan ranges are not yet set"); 
      if (scanCompleted_)
         return null; 
      retArray = new int[4];
      switch (lastBufCompleted_) {
         case -1:
         case 1:
            bytesRead = hdfsClient_[0].trafHdfsReadBuffer(); 
            bufNo = 0;
            rangeNo = hdfsClient_[0].getRangeNo();
            isEOF = hdfsClient_[0].isEOF();
            break;
         case 0:
            bytesRead = hdfsClient_[1].trafHdfsReadBuffer(); 
            bufNo = 1;
            rangeNo = hdfsClient_[1].getRangeNo();
            isEOF = hdfsClient_[1].isEOF();
            break;
         default:
            bufNo = -1;
            bytesRead = -1;
            rangeNo = -1;
            isEOF = 0;
      }    
      retArray[0] = bytesRead;
      retArray[1] = bufNo;
      retArray[2] = rangeNo; 
      retArray[3] = isEOF;
      if (logger_.isDebugEnabled())
         logger_.debug(" Range No " + retArray[2] + " Buffer No " + retArray[1] + " Bytes Read " + retArray[0] + " isEOF " + retArray[3]); 
      lastBufCompleted_ = bufNo;
      if (isEOF == 1) {
         if (currRange_ == (hdfsScanRanges_.length-1)) {
            scanCompleted_ = true;
            return retArray;
         } else {
            currRange_++;
            currPos_ = hdfsScanRanges_[currRange_].pos_;
            lenRemain_ = hdfsScanRanges_[currRange_].len_;
            bytesRead = 0;
         }
      }
      switch (lastBufCompleted_)
      {
         case 0:
            hdfsScanRange(1, bytesRead);
            break;
         case 1:
            hdfsScanRange(0, bytesRead);
            break;            
         default:
            break;
      }
      return retArray;
   } 
   
   public static void shutdown() throws InterruptedException
   {
      HDFSClient.shutdown();
   }
   public static void main(String[] args) throws Exception
   {

      if (args.length < 3)
      {
         System.out.println("Usage: org.trafodion.sql.HdfsScan <tableName> <buffer_length> <number_of_splits>");
         return;
      }
      String tableName = args[0];
      int capacity = Integer.parseInt(args[1]) * 1024 *1024;
      int split = Integer.parseInt(args[2]);
      HiveConf config = new HiveConf(); 
      HiveMetaStoreClient hiveMeta = new HiveMetaStoreClient(config); 
      Table table = hiveMeta.getTable(tableName);
      StorageDescriptor sd = table.getSd();
      String location = sd.getLocation();
      URI uri = new URI(location);
      Path path = new Path(uri);
      FileSystem fs = FileSystem.get(config);       
      FileStatus file_status[] = fs.listStatus(path);
      ByteBuffer buf1 = ByteBuffer.allocateDirect(capacity);
      ByteBuffer buf2 = ByteBuffer.allocateDirect(capacity);
      String fileName[] = new String[file_status.length * split];
      long pos[] = new long[file_status.length * split];
      long len[] = new long[file_status.length * split];
      int range[] = new int[file_status.length * split];
      for (int i = 0 ; i < file_status.length * split; i++) {
         Path filePath = file_status[i].getPath();
         long fileLen = file_status[i].getLen(); 
         long splitLen = fileLen / split;
         fileName[i] = filePath.toString();
         System.out.println (" fileName " + fileName[i] + " Length " + fileLen); 
         long splitPos = 0;
         for (int j = 0 ; j < split ; j++)
         { 
            fileName[i] = filePath.toString();
            pos[i] = splitPos + (splitLen * j);
            len[i] = splitLen;
            range[i] = i;
            if (j == (split-1))
               len[i] = fileLen - (splitLen *(j));
            System.out.println ("Range " + i + " Pos " + pos[i] + " Length " + len[i]); 
            i++;
         }
      }
      long time1 = System.currentTimeMillis();
      HdfsScan hdfsScan = new HdfsScan();
      hdfsScan.setScanRanges(buf1, buf2, fileName, pos, len, range);
      int[] retArray;
      int bytesCompleted;
      ByteBuffer buf;
      while (true) {
         retArray = hdfsScan.trafHdfsRead();
         if (retArray == null)
            break;
         System.out.println("Range No:" + retArray[2] + " Buf No:" + retArray[1] + " Bytes Completed:" + retArray[0] + " EOF:" + retArray[3]);
         if (retArray[1] == 0)
            buf = buf1;
         else
            buf = buf2; 
         buf.position(0);
         for (int i = 0; i < 50; i++)
           System.out.print(buf.get());
         System.out.println("");
      }
      long time2 = System.currentTimeMillis();
      HdfsScan.shutdown();
      System.out.println("Time taken in milliSeconds " + (time2-time1) );
   }
}

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

import java.io.IOException;
import java.io.EOFException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileStatus;
import java.io.EOFException;
import java.util.concurrent.Callable;
import java.util.concurrent.Future;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import org.apache.hadoop.io.compress.CodecPool;
import org.apache.hadoop.io.compress.CompressionCodec;
import org.apache.hadoop.io.compress.Compressor;
import org.apache.hadoop.io.compress.GzipCodec;
import org.apache.hadoop.io.SequenceFile.CompressionType;
import org.apache.hadoop.util.ReflectionUtils;

public class HDFSClient 
{
   static Logger logger_ = Logger.getLogger(HDFSClient.class.getName());
   private static Configuration config_ = null;
   private static ExecutorService executorService_ = null;
   private static FileSystem defaultFs_ = null;
   private FileSystem fs_ = null;
   private int bufNo_;
   private int rangeNo_;
   private FSDataInputStream fsdis_; 
   private OutputStream outStream_;
   private String filename_;
   private ByteBuffer buf_;
   private int bufLen_;
   private int bufOffset_ = 0;
   private long pos_ = 0;
   private int len_ = 0;
   private int lenRemain_ = 0; 
   private int blockSize_; 
   private int bytesRead_;
   private Future future_ = null;
   private int isEOF_ = 0; 
   static {
      String confFile = System.getProperty("trafodion.log4j.configFile");
      System.setProperty("trafodion.root", System.getenv("TRAF_HOME"));
      if (confFile == null) {
         confFile = System.getenv("TRAF_CONF") + "/log4j.sql.config";
      }
      PropertyConfigurator.configure(confFile);
      config_ = TrafConfiguration.create(TrafConfiguration.HDFS_CONF);
      executorService_ = Executors.newCachedThreadPool();
      try {
         defaultFs_ = FileSystem.get(config_);
      }
      catch (IOException ioe) {
         throw new RuntimeException("Exception in HDFSClient static block", ioe);
      }
      System.loadLibrary("executor");
   }

   class HDFSRead implements Callable 
   {
      HDFSRead() 
      {
      }
 
      public Object call() throws IOException 
      {
         int bytesRead;
         int totalBytesRead = 0;
         if (! buf_.hasArray()) {
            try {
              fsdis_.seek(pos_);
            } catch (EOFException e) {
              isEOF_ = 1;
              return new Integer(totalBytesRead);
            } 
         }
         do
         {
            if (buf_.hasArray())
               bytesRead = fsdis_.read(pos_, buf_.array(), bufOffset_, lenRemain_);
            else 
               bytesRead = fsdis_.read(buf_);
            if (bytesRead == -1) {
               isEOF_ = 1;
               break;
            }
            if (bytesRead == 0)
               break;
            totalBytesRead += bytesRead;          
            if (totalBytesRead == bufLen_)
                break;
            bufOffset_ += bytesRead;
            pos_ += bytesRead;
            lenRemain_ -= bytesRead;
         } while (lenRemain_ > 0);
         return new Integer(totalBytesRead);
      }
   }
       
   public HDFSClient() 
   {
   }

   // This constructor enables the hdfs data to be read in another thread while the previously 
   // read buffer is being processed by the SQL engine 
   public HDFSClient(int bufNo, int rangeNo, String filename, ByteBuffer buffer, long position, int length) throws IOException
   {
      bufNo_ = bufNo; 
      rangeNo_ = rangeNo;
      filename_ = filename;
      Path filepath = new Path(filename_);
      fs_ = FileSystem.get(filepath.toUri(),config_);
      fsdis_ = fs_.open(filepath);
      blockSize_ = (int)fs_.getDefaultBlockSize(filepath);
      buf_  = buffer;
      bufOffset_ = 0;
      pos_ = position;
      len_ = length;
      if (buffer.hasArray()) 
         bufLen_ = buffer.array().length;
      else {
         bufLen_ = buffer.capacity();
         buf_.position(0);
      }
      lenRemain_ = (len_ > bufLen_) ? bufLen_ : len_;
      if (lenRemain_ != 0) {
         future_ = executorService_.submit(new HDFSRead());
      }
   }

   public int trafHdfsReadBuffer() throws IOException, InterruptedException, ExecutionException
   {
      Integer retObject = 0;
      int bytesRead;
      retObject = (Integer)future_.get();
      bytesRead = retObject.intValue();
      fsdis_.close();
      return bytesRead;
   }  

   public int getRangeNo()
   {
      return rangeNo_;
   }
  
   public int isEOF()
   {
      return isEOF_;
   }

   boolean hdfsCreate(String fname , boolean compress) throws IOException
   {
     if (logger_.isDebugEnabled()) 
        logger_.debug("HDFSClient.hdfsCreate() - started" );
      Path filePath = null;
      if (!compress || (compress && fname.endsWith(".gz")))
        filePath = new Path(fname);
      else
        filePath = new Path(fname + ".gz");
        
      FileSystem fs = FileSystem.get(filePath.toUri(),config_);
      FSDataOutputStream fsOut = fs.create(filePath, true);
      
      if (compress) {
        GzipCodec gzipCodec = (GzipCodec) ReflectionUtils.newInstance( GzipCodec.class, config_);
        Compressor gzipCompressor = CodecPool.getCompressor(gzipCodec);
        outStream_= gzipCodec.createOutputStream(fsOut, gzipCompressor);
      }
      else
        outStream_ = fsOut;      
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsCreate() - compressed output stream created" );
      return true;
    }

    boolean hdfsOpen(String fname , boolean compress) throws IOException
    {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsOpen() - started" );
      Path filePath = null;
      if (!compress || (compress && fname.endsWith(".gz")))
        filePath = new Path(fname);
      else
        filePath = new Path(fname + ".gz");
        
      FileSystem fs = FileSystem.get(filePath.toUri(),config_);
      FSDataOutputStream fsOut;
      if (fs.exists(filePath))
         fsOut = fs.append(filePath);
      else
         fsOut = fs.create(filePath);
      
      if (compress) {
        GzipCodec gzipCodec = (GzipCodec) ReflectionUtils.newInstance( GzipCodec.class, config_);
        Compressor gzipCompressor = CodecPool.getCompressor(gzipCodec);
        outStream_= gzipCodec.createOutputStream(fsOut, gzipCompressor);
      }
      else
        outStream_ = fsOut;      
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsCreate() - compressed output stream created" );
      return true;
    }
    
    boolean hdfsWrite(byte[] buff, long len) throws IOException
    {

      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsWrite() - started" );
      outStream_.write(buff);
      outStream_.flush();
      if (logger_.isDebugEnabled()) logger_.debug("HDFSClient.hdfsWrite() - bytes written and flushed:" + len  );
      return true;
    }
    
    boolean hdfsClose() throws IOException
    {
      if (logger_.isDebugEnabled()) logger_.debug("HDFSClient.hdfsClose() - started" );
      if (outStream_ != null) {
          outStream_.close();
          outStream_ = null;
      }
      return true;
    }

    
    public static boolean hdfsMergeFiles(String srcPathStr, String dstPathStr) throws IOException
    {
      if (logger_.isDebugEnabled()) logger_.debug("HDFSClient.hdfsMergeFiles() - start");
      if (logger_.isDebugEnabled()) logger_.debug("HDFSClient.hdfsMergeFiles() - source Path: " + srcPathStr + 
                                               ", destination File:" + dstPathStr );
        Path srcPath = new Path(srcPathStr );
        srcPath = srcPath.makeQualified(srcPath.toUri(), null);
        FileSystem srcFs = FileSystem.get(srcPath.toUri(),config_);
  
        Path dstPath = new Path(dstPathStr);
        dstPath = dstPath.makeQualified(dstPath.toUri(), null);
        FileSystem dstFs = FileSystem.get(dstPath.toUri(),config_);
        
        if (dstFs.exists(dstPath))
        {
          if (logger_.isDebugEnabled()) logger_.debug("HDFSClient.hdfsMergeFiles() - destination files exists" );
          // for this prototype we just delete the file-- will change in next code drops
          dstFs.delete(dstPath, false);
           // The caller should already have checked existence of file-- throw exception 
           //throw new FileAlreadyExistsException(dstPath.toString());
        }
        
        Path tmpSrcPath = new Path(srcPath, "tmp");

        FileSystem.mkdirs(srcFs, tmpSrcPath,srcFs.getFileStatus(srcPath).getPermission());
        logger_.debug("HDFSClient.hdfsMergeFiles() - tmp folder created." );
        Path[] files = FileUtil.stat2Paths(srcFs.listStatus(srcPath));
        for (Path f : files)
        {
          srcFs.rename(f, tmpSrcPath);
        }
        // copyMerge and use false for the delete option since it removes the whole directory
        if (logger_.isDebugEnabled()) logger_.debug("HDFSClient.hdfsMergeFiles() - copyMerge" );
        FileUtil.copyMerge(srcFs, tmpSrcPath, dstFs, dstPath, false, config_, null);
        
        if (logger_.isDebugEnabled()) logger_.debug("HDFSClient.hdfsMergeFiles() - delete intermediate files" );
        srcFs.delete(tmpSrcPath, true);
      return true;
    }

   public static boolean hdfsCleanUnloadPath(String uldPathStr
                         /*, boolean checkExistence, String mergeFileStr*/) throws IOException
   {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsCleanUnloadPath() - unload Path: " + uldPathStr );
      
      Path uldPath = new Path(uldPathStr );
      FileSystem fs = FileSystem.get(uldPath.toUri(), config_);
      if (!fs.exists(uldPath))
      {
        //unload location does not exist. hdfscreate will create it later
        //nothing to do 
        return true;
      }
       
      Path[] files = FileUtil.stat2Paths(fs.listStatus(uldPath));
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsCleanUnloadPath() - delete files" );
      for (Path f : files){
        fs.delete(f, false);
      }
      return true;
   }

   public static boolean hdfsExists(String filePathStr) throws IOException 
   {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsExists() - Path: " + filePathStr);

      Path filePath = new Path(filePathStr );
      FileSystem fs = FileSystem.get(filePath.toUri(), config_);
      if (fs.exists(filePath)) 
         return true;
      return false;
   }

   public static boolean hdfsDeletePath(String pathStr) throws IOException
   {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsDeletePath() - start - Path: " + pathStr);
      Path delPath = new Path(pathStr );
      FileSystem fs = FileSystem.get(delPath.toUri(), config_);
      fs.delete(delPath, true);
      return true;
   }

   public int hdfsListDirectory(String pathStr, long hdfsClientJniObj) throws IOException
   {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsListDirectory() - start - Path: " + pathStr);
      Path listPath = new Path(pathStr );
      FileSystem fs = FileSystem.get(listPath.toUri(), config_);
      FileStatus[] fileStatus;
      if (fs.isDirectory(listPath)) 
         fileStatus = fs.listStatus(listPath); 
      else
         throw new IOException("The path " + listPath + "is not a directory");
      FileStatus aFileStatus; 
      int retcode;
      if (fileStatus != null) {
         for (int i = 0; i < fileStatus.length; i++)
         {
             aFileStatus = fileStatus[i];
             retcode = sendFileStatus(hdfsClientJniObj, fileStatus.length, 
                            i,
                            aFileStatus.isDirectory(),
                            aFileStatus.getPath().toString(),
                            aFileStatus.getModificationTime(),
                            aFileStatus.getLen(),
                            aFileStatus.getReplication(),
                            aFileStatus.getBlockSize(),
                            aFileStatus.getOwner(),
                            aFileStatus.getGroup(),
                            aFileStatus.getPermission().toShort(),
                            aFileStatus.getAccessTime());          
             if (retcode != 0)
                throw new IOException("Error " + retcode + " while sending the file status info for file " + aFileStatus.getPath().toString());
         }
         return fileStatus.length;
      }
      else  
         return 0;
   }
 
   public static void shutdown() throws InterruptedException
   {
      executorService_.awaitTermination(100, TimeUnit.MILLISECONDS);
      executorService_.shutdown();
   }
   
   private native int sendFileStatus(long jniObj, int numFiles, int fileNo, boolean isDir, 
                        String filename, long modTime, long len,
                        short numReplicas, long blockSize, String owner, String group,
                        short permissions, long accessTime);

}



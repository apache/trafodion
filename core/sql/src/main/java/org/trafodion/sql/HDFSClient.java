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
import java.io.FileNotFoundException;
import java.io.EOFException;
import java.io.OutputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.PathFilter;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.io.compress.CompressionInputStream;
import org.apache.hadoop.io.SequenceFile;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.io.Writable;

import java.io.EOFException;
import java.util.concurrent.Callable;
import java.util.concurrent.Future;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import org.apache.hadoop.io.compress.CodecPool;
import org.apache.hadoop.io.compress.CompressionCodec;
import org.apache.hadoop.io.compress.Compressor;
import org.apache.hadoop.io.compress.GzipCodec;
import org.apache.hadoop.io.SequenceFile.CompressionType;
import org.apache.hadoop.util.ReflectionUtils;
import org.apache.hadoop.io.compress.CompressionCodecFactory;

//
//  To read a range in a Hdfs file, use the constructor
//   public HDFSClient(int bufNo, int rangeNo, String filename, ByteBuffer buffer, long position, int length, CompressionInputStream inStream) 
// 
//  For instance methods like hdfsListDirectory use the constructor
//     public HDFSClient()
//
//  For all static methods use
//     HDFSClient::<static_method_name>
//

public class HDFSClient 
{
   // Keep the constants and string array below in sync with 
   // enum CompressionMethod at sql/comexe/ComCompressionInfo.h
   static final short UNKNOWN_COMPRESSION = 0;
   static final short UNCOMPRESSED = 1;
   static final short LZOP = 5;
   static final String COMPRESSION_TYPE[] = {
      "UNKNOWN_COMPRESSION", // unable to determine compression method
      "UNCOMPRESSED",            // file is not compressed
      "LZO_DEFLATE",             // using LZO deflate compression
      "DEFLATE",                 // using DEFLATE compression
      "GZIP",                    // using GZIP compression
      "LZOP"};                   // using LZOP compression
   static Logger logger_ = Logger.getLogger(HDFSClient.class.getName());
   private static Configuration config_ = null;
   private static ExecutorService executorService_ = null;
   private static FileSystem defaultFs_ = null;
   private static CompressionCodecFactory codecFactory_ = null;
   private static boolean alluxioNotInstalled_ = false;
   private FileSystem fs_ = null;
   private int bufNo_;
   private int rangeNo_;
   private FSDataInputStream fsdis_;
           CompressionInputStream inStream_; 
   private OutputStream outStream_;
   private String filename_;
   private ByteBuffer buf_;
   private ByteBuffer savedBuf_;
   private byte[] bufArray_;
   private int bufLen_;
   private int bufOffset_ = 0;
   private long pos_ = 0;
   private int len_ = 0;
   private int lenRemain_ = 0; 
   private int blockSize_; 
   private int bytesRead_;
   private Future future_ = null;
   private int isEOF_ = 0; 
   private int totalBytesWritten_ = 0;
   private Path filepath_ = null;
   boolean compressed_ = false;
   private CompressionCodec codec_ = null;
   private short compressionType_;
   private int ioByteArraySizeInKB_;

   private boolean sequenceFile_;
   private byte recDelimiter_;
   private Writable key_;
   private Writable value_;
   private SequenceFile.Reader reader_;
   private SequenceFile.CompressionType seqCompressionType_;

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
      try {
         boolean alluxioFs = defaultFs_ instanceof alluxio.hadoop.FileSystem;
      }
      catch (Throwable rte)
      {
         // Ignore the exception. It is not needed for alluxio to be installed
         // for the methods of this class to work if 
         // alluxio filesystem is NOT required
         alluxioNotInstalled_ = true;
      }
      codecFactory_ = new CompressionCodecFactory(config_); 
      System.loadLibrary("executor");
   }

   // The object instance that runs in the threadpool to read
   // the requested chunk in the range

   // FSDataInputStream.read method may not read the requested length in one shot
   // Loop to read the requested length or EOF is reached 
   // Requested length can never be larger than the buffer size

   class HDFSRead implements Callable 
   {
      HDFSRead() 
      {
      }
    
      public Object call() throws IOException 
      {
         int bytesRead;
         int totalBytesRead = 0;
         if (sequenceFile_) {
            // do nothing
            ; 
         } else if (compressed_) {
            bufArray_ = new byte[ioByteArraySizeInKB_ * 1024];
         } 
         else  {
            // alluxio doesn't support direct ByteBuffer reads
            // Hence, create a non-direct ByteBuffer, read into
            // byteArray backing up this ByteBuffer and 
            // then copy the data read to direct ByteBuffer for the 
            // native layer to process the data
            if ((! alluxioNotInstalled_) && fs_ instanceof alluxio.hadoop.FileSystem) {
               savedBuf_ = buf_;
               buf_ = ByteBuffer.allocate(savedBuf_.capacity());
            }
            if (! buf_.hasArray()) {
               try {
                  fsdis_.seek(pos_);
               } catch (EOFException e) {
                  isEOF_ = 1;
                  return new Integer(totalBytesRead);
               } 
            }
         }
         do
         {
            if (sequenceFile_) 
               bytesRead = sequenceFileRead(lenRemain_);
            else if (compressed_) 
               bytesRead = compressedFileRead(lenRemain_);
            else {
               if (buf_.hasArray())
                  bytesRead = fsdis_.read(pos_, buf_.array(), bufOffset_, lenRemain_);
               else 
                  bytesRead = fsdis_.read(buf_);
            }
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
         if ((! alluxioNotInstalled_) && fs_ instanceof alluxio.hadoop.FileSystem) {
            if (totalBytesRead > 0) {
               byte[] temp = buf_.array();
               savedBuf_.put(temp, 0, totalBytesRead);
            }
         }
         return new Integer(totalBytesRead);
      }
    } 

    int compressedFileRead(int readLenRemain) throws IOException 
    {
       int totalReadLen = 0;
       int readLen;
       int offset = 0;
       int retcode;

         int lenRemain = ((readLenRemain > bufArray_.length) ? bufArray_.length : readLenRemain);
         do 
         {
            readLen = inStream_.read(bufArray_, offset, lenRemain);
            if (readLen == -1 || readLen == 0)
               break;
            totalReadLen += readLen;
            offset  += readLen;
            lenRemain -= readLen;
         } while (lenRemain > 0);
         if (totalReadLen > 0) {
            if ((retcode = copyToByteBuffer(buf_, bufOffset_, bufArray_, totalReadLen)) != 0)
               throw new IOException("Failure to copy to the DirectByteBuffer in the native layer with error code " + retcode);
         }
         else
            totalReadLen = -1;
         return totalReadLen; 
    } 

    /* Trafodion adds record delimiter '\n' while copying it
       to buffer backing up the ByteBuffer */

    int sequenceFileRead(int readLenRemain) throws IOException 
    {
       boolean eof = false;
       boolean nextValue;
       byte[] byteArray;
       int readLen;
       int totalReadLen = 0;
       long tempPos;
       int lenRemain = readLenRemain;

       while (!eof && lenRemain > 0) {
          try {
            tempPos = reader_.getPosition();
            nextValue = reader_.next(key_, value_);
            if (!nextValue) {
               eof = true;
               break;
            }
          }
          catch (java.io.EOFException e)
          {
              eof = true;
              break;
          }
          byteArray = ((Text)value_).getBytes();
          readLen = ((Text)value_).getLength();
          if ((readLen+1) <= lenRemain) {
                            
              buf_.put(byteArray, 0, readLen);
              buf_.put(recDelimiter_);
              lenRemain -= (readLen+1);
              totalReadLen += (readLen+1);
          } else {
              // Reset the position because the row can't be copied to buffer
              try {
                reader_.sync(tempPos); 
              }
              catch (java.io.EOFException e1)
              {}
              break;       
          }   
       }
       if (totalReadLen == 0)
          totalReadLen = -1;  
       return totalReadLen;
    } 

   native int copyToByteBuffer(ByteBuffer buf, int bufOffset, byte[] bufArray, int copyLen);
       
   public HDFSClient() 
   {
   }

   // This constructor enables the hdfs data to be read in another thread while the previously 
   // read buffer is being processed by the SQL engine 
   // Opens the file and hands over the needed info to HdfsRead instance to read 
   // The passed in length can never be more than the size of the buffer
   // If the range has a length more than the buffer length, the range is chunked
   // in HdfsScan
   public HDFSClient(int bufNo, int ioByteArraySizeInKB, int rangeNo, String filename, ByteBuffer buffer, long position, 
                int length, short compressionType, boolean sequenceFile, byte recDelimiter, CompressionInputStream inStream) 
                throws IOException, EOFException
   {
      bufNo_ = bufNo; 
      rangeNo_ = rangeNo;
      filename_ = filename;
      ioByteArraySizeInKB_ = ioByteArraySizeInKB;
      filepath_ = new Path(filename_);
      fs_ = FileSystem.get(filepath_.toUri(),config_);
      compressionType_ = compressionType;
      inStream_ = inStream;
      sequenceFile_ = sequenceFile;
      recDelimiter_ = recDelimiter;
      if (sequenceFile_) {
         fsdis_ = null;
         inStream_ = null;
      } 
      else {
         codec_ = codecFactory_.getCodec(filepath_);
         if (codec_ != null) {
            compressed_ = true;
            if (inStream_ == null)
               inStream_ = codec_.createInputStream(fs_.open(filepath_));
         }
         else {
           if ((compressionType_ != UNCOMPRESSED) && (compressionType_ != UNKNOWN_COMPRESSION))
              throw new IOException(COMPRESSION_TYPE[compressionType_] + " compression codec is not configured in Hadoop");
           if (filename_.endsWith(".lzo"))
              throw new IOException(COMPRESSION_TYPE[LZOP] + " compression codec is not configured in Hadoop");
           fsdis_ = fs_.open(filepath_);
         }
      }
      blockSize_ = (int)fs_.getDefaultBlockSize(filepath_);
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
         if (sequenceFile_) 
            initSequenceFileRead();
         future_ = executorService_.submit(new HDFSRead());
      }
   }

   /* Trafodion support Sequence file with keys written via ByteWritble or Text class
      and value written via Text class. However, the key is completely ignored
      while reading the rows. The columns in the value is delimited by column delimiter 001(octal).
   */ 

   public void initSequenceFileRead() throws IOException, EOFException
   {
      SequenceFile.Reader.Option seqLen = SequenceFile.Reader.length(lenRemain_);
      SequenceFile.Reader.Option seqFileName = SequenceFile.Reader.file(filepath_);
      reader_ = new SequenceFile.Reader(config_, seqLen, seqFileName);
      seqCompressionType_ = reader_.getCompressionType();
      if (seqCompressionType_ == SequenceFile.CompressionType.NONE)
         compressed_ = false;
      else
         compressed_ = true;
      if (compressed_ && pos_ != 0)
         throw new IOException("Spliting of compressed sequence file is not supported");
      try {
        reader_.sync(pos_);
      } catch (EOFException e)
      {}
      
      String keyClass = reader_.getKeyClassName(); 
      String valueClass = reader_.getValueClassName();
      if (! valueClass.equals("org.apache.hadoop.io.Text")) 
         throw new IOException("Sequence File with the value class of type " + valueClass +  " is not supported");
      if (!(keyClass.equals("org.apache.hadoop.io.Text") || keyClass.equals("org.apache.hadoop.io.BytesWritable")))
         throw new IOException("Sequence File with the key class of type " + keyClass + " is not supported");
      key_ = (Writable) ReflectionUtils.newInstance(reader_.getKeyClass(), config_);
      value_ = (Writable) ReflectionUtils.newInstance(reader_.getValueClass(), config_);
   }

  //  This method waits for the read to complete. Read can complete due to one of the following
  //  a) buffer is full
  //  b) EOF is reached
  //  c) An exception is encountered while reading the file
   public int trafHdfsReadBuffer() throws IOException, InterruptedException, ExecutionException
   {
      Integer retObject = 0;
      int bytesRead;
      retObject = (Integer)future_.get();
      bytesRead = retObject.intValue();
      if (! compressed_ && fsdis_ != null)
         fsdis_.close();
      fsdis_ = null;
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

   boolean hdfsCreate(String fname , boolean overwrite, boolean append, boolean compress) throws IOException
   {
      boolean fileExists = false;      
      filename_ = fname;
      if (logger_.isDebugEnabled()) 
        logger_.debug("HDFSClient.hdfsCreate() - started" );
      if (!compress || (compress && fname.endsWith(".gz")))
        filepath_ = new Path(fname);
      else
        filepath_ = new Path(fname + ".gz");
        
      fs_ = FileSystem.get(filepath_.toUri(),config_);
      compressed_ = compress;
      fsdis_ = null;      
      if (fs_.exists(filepath_))
      {
         if (overwrite)
            fs_.delete(filepath_);
         else if (!append)
            throw new IOException(filepath_ + " already exists");
         else
            fileExists = true;
      }
      FSDataOutputStream fsOut = null;
      if (!fileExists) {
         fsOut = fs_.create(filepath_);
         fsOut.close();
      }
      return true;
   } 

   boolean hdfsOpen(String fname , boolean compress) throws IOException
   {
      filename_ = fname;
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsOpen() - started" );
      if (!compress || (compress && fname.endsWith(".gz")))
        filepath_ = new Path(fname);
      else
        filepath_ = new Path(fname + ".gz");
      fs_ = FileSystem.get(filepath_.toUri(),config_);
      compressed_ = compress;  
      outStream_ = null;
      fsdis_ = null;      
      return true;
    }

    long hdfsSize() throws IOException
    {
       FileStatus filestatus;
       try 
       {
          filestatus = fs_.getFileStatus(filepath_);
       } catch (java.io.FileNotFoundException e)
       {
          return 0;
       }
       if (filestatus.isFile())
          return filestatus.getLen();
       else
          return -1;
    }

    long hdfsWriteImmediate(byte[] buff, boolean doRetry) throws IOException
    {

      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsWriteImmediate() - started" );
   
        FSDataOutputStream fsOut=null;
        FileStatus filestatus = null;
        long writeOffset=0;
        long writeEndOffset =0;
        int trycount = 0;
        
        while (trycount < 3)
        {
          try
          { 
            if (logger_.isDebugEnabled()) 
                logger_.debug("HDFSClient.hdfsWriteImmediate() - started" );
  
            if (fs_.exists(filepath_)) {
                filestatus = fs_.getFileStatus(filepath_);
                fsOut = fs_.append(filepath_);
                 writeOffset = filestatus.getLen(); 
             }
             else {
                   fsOut = fs_.create(filepath_);
                   writeOffset = 0;
             }
             fsOut.write(buff);
             fsOut.hflush();
             writeEndOffset = fsOut.getPos(); 
             } finally {
                if (fsOut != null )
                   fsOut.close();
           }
           // Do a checksum to ensure writeOffset is really the right value.
          if (doRetry && (buff.length != writeEndOffset-writeOffset))
            {
               trycount++;
               if (trycount == 3)
                 {
                    throw new IOException("HDFSwrite did not succeed due to multiple concurrent writes ");    
                 }
             }
           else trycount = 100; //if doRetry is false or writeOffset is ok
        } //while
      return writeOffset;
    }
    
    int hdfsWrite(byte[] buff) throws IOException
    {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsWrite() - started" );

      FSDataOutputStream fsOut;
      if (outStream_ == null) {
         if (fs_.exists(filepath_))
            fsOut = fs_.append(filepath_);
         else
            fsOut = fs_.create(filepath_);
      
         if (compressed_) {
            GzipCodec gzipCodec = (GzipCodec) ReflectionUtils.newInstance( GzipCodec.class, config_);
            Compressor gzipCompressor = CodecPool.getCompressor(gzipCodec);
            outStream_= gzipCodec.createOutputStream(fsOut, gzipCompressor);
         }
         else
            outStream_ = fsOut;      
         if (logger_.isDebugEnabled()) 
            logger_.debug("HDFSClient.hdfsWrite() - output stream created" );
      }
      outStream_.write(buff);
      if (outStream_ instanceof FSDataOutputStream)
         ((FSDataOutputStream)outStream_).hsync();
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsWrite() - bytes written " + buff.length);
      return buff.length;
    }

    int hdfsRead(long pos, ByteBuffer buffer) throws IOException
    {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsRead() - started" );
      if (fsdis_ == null && inStream_ == null ) {
         try {
         codec_ = codecFactory_.getCodec(filepath_);
         if (codec_ != null) {
            compressed_ = true;
            inStream_ = codec_.createInputStream(fs_.open(filepath_));
         }
         else
            fsdis_ = fs_.open(filepath_);
         } catch (java.io.FileNotFoundException e) {
            return 0;
         }
      }
      int lenRemain;   
      int bytesRead;
      int totalBytesRead = 0;
      int bufLen;
      int bufOffset = 0;
      if (compressed_) {
         if (pos != 0 && pos != -1)
            throw new IOException("Compressed files can be read from a non-zero position");
         else
            pos_ = 0;
      }
      else
      if (pos != -1) 
         pos_ = pos;
      if (compressed_ && bufArray_ != null) 
         bufArray_ = new byte[ioByteArraySizeInKB_ * 1024];
      if (buffer.hasArray())
         bufLen = buffer.array().length;
      else
      {
         if (pos_ != -1)
            fsdis_.seek(pos_);
         bufLen = buffer.capacity();
      }
      lenRemain = bufLen;
      do
      {
         if (compressed_) {
            bytesRead = compressedFileRead(lenRemain);
         } else {
           if (buffer.hasArray()) 
              bytesRead = fsdis_.read(pos_, buffer.array(), bufOffset, lenRemain);
           else
              bytesRead = fsdis_.read(buffer);    
         }
         if (bytesRead == -1 || bytesRead == 0)
            break;    
         totalBytesRead += bytesRead;
         pos_ += bytesRead;
         lenRemain -= bytesRead;
      } while (lenRemain > 0);
      return totalBytesRead;
    }
    
    boolean hdfsClose() throws IOException
    {
      if (logger_.isDebugEnabled()) logger_.debug("HDFSClient.hdfsClose() - started" );
      if (outStream_ != null) {
          outStream_.close();
          outStream_ = null;
      }
      if (fsdis_ != null)
         fsdis_.close();
      return true;
    }

    static long hdfsSize(String filename) throws IOException
    {
       Path filepath = new Path(filename);
       FileSystem fs = FileSystem.get(filepath.toUri(),config_);
       FileStatus filestatus;
       try
       {
          filestatus = fs.getFileStatus(filepath);
       } catch (java.io.FileNotFoundException e)
       {
          return 0;
       } 
       if (filestatus.isFile())
          return filestatus.getLen();
       else
          return -1;
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

   public static boolean hdfsDeleteFiles(String dirPathStr, String startingFileName) throws IOException
   {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsDeleteFiles(" + dirPathStr + ", " + startingFileName +")");

      Path dirPath = new Path(dirPathStr );
      FileSystem fs = FileSystem.get(dirPath.toUri(), config_);
      FileStatus[] fileStatus;
      if (fs.isDirectory(dirPath))
         fileStatus = fs.listStatus(dirPath);
      else
         throw new IOException("The path " + dirPath + " is not a directory");
      FileStatus aFileStatus; 
      if (fileStatus != null) {
         for (int i = 0; i < fileStatus.length; i++)
         {
             aFileStatus = fileStatus[i];
             if (! aFileStatus.isDirectory()) {
                String pathName =  aFileStatus.getPath().toString();
                String filenameParts[] = pathName.split(dirPathStr);
                if (filenameParts.length == 2 && filenameParts[1].startsWith(startingFileName))
                   fs.delete(aFileStatus.getPath());
             }
         }
      }
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

   
   public void stop() throws IOException
   {
      if (future_ != null) {
         try {
           future_.get(30, TimeUnit.SECONDS);
         } catch(TimeoutException e) {
            logger_.error("Asynchronous Thread of HdfsScan is Cancelled (timeout), ", e);
            future_.cancel(true);
        } catch(InterruptedException e) {
            logger_.error("Asynchronous Thread of HdfsScan is Cancelled (interrupt), ", e);
            future_.cancel(true); // Interrupt the thread
        } catch (ExecutionException ee)
        {
        }
        future_ = null;
      }
   }
 
   public static void shutdown() throws InterruptedException
   {
      executorService_.awaitTermination(100, TimeUnit.MILLISECONDS);
      executorService_.shutdown();
   }
   
   private static FileSystem getFileSystem() throws IOException
   {
       return defaultFs_;
   }

   // if levelDeep = 0, return the max modification timestamp of the passed-in HDFS URIs
   // (a tab-separated list of 0 or more paths)
   // if levelDeep > 0, also check all directories "levelDeep" levels below. Exclude
   // directories that start with a dot (hidden directories)
   public static long getHiveTableMaxModificationTs( String stableDirPaths, int levelDeep) throws FileNotFoundException, IOException
   {
       long result = 0;
       if (logger_.isDebugEnabled())
          logger_.debug("HDFSClient:getHiveTableMaxModificationTs enter");

       String[] tableDirPaths = stableDirPaths.split("\t");
       // account for root dir
       for (int i=0; i<tableDirPaths.length; i++) {
           FileStatus r = getFileSystem().getFileStatus(new Path(tableDirPaths[i]));// super fast API, return in .2ms
           if (r != null && r.getModificationTime() > result)
               result = r.getModificationTime();
       }

       if (levelDeep>0)
       {
           Path[] paths = new Path[tableDirPaths.length];
           for (int i=0; i<tableDirPaths.length; i++)
               paths[i] = new Path(tableDirPaths[i]);
           long l = getHiveTableMaxModificationTs2(paths,levelDeep);
           if (l > result)
              result = l;
       }
       if (logger_.isDebugEnabled())
           logger_.debug("HDFSClient:getHiveTableMaxModificationTs "+stableDirPaths+" levelDeep"+levelDeep+":"+result);
       return result;
   }

   private static long getHiveTableMaxModificationTs2(Path[] paths, int levelDeep)throws FileNotFoundException, IOException
   {
       long result = 0;
       PathFilter filter = new PathFilter(){
           public boolean accept(Path file){
             return !file.getName().startsWith(".");//filter out hidden files and directories
           }
       };
       FileStatus[] fileStatuss=null;
       if (levelDeep == 1){ // stop condition on recursive function
           //check parent level (important for deletes):
           for (Path path : paths){
               FileStatus r = getFileSystem().getFileStatus(path);// super fast API, return in .2ms
               if (r != null && r.getModificationTime()>result)
                   result = r.getModificationTime();
           }
           if (paths.length==1)
               fileStatuss = getFileSystem().listStatus(paths[0],filter);// minor optimization. avoid using list based API when not needed
           else
               fileStatuss = getFileSystem().listStatus(paths,filter);
           for(int i=0;i<fileStatuss.length;i++)
               if (fileStatuss[i].isDirectory() && fileStatuss[i].getModificationTime()>result)
                   result = fileStatuss[i].getModificationTime();
       }else{//here levelDeep >1
           List<Path> pathList = new ArrayList<Path>();
           if (paths.length==1)
               fileStatuss = getFileSystem().listStatus(paths[0],filter);// minor optimization. avoid using list based API when not needed
           else
               fileStatuss = getFileSystem().listStatus(paths,filter);
           for(int i=0;i<fileStatuss.length;i++)
               if (fileStatuss[i].isDirectory())
               {
                   pathList.add(fileStatuss[i].getPath());
                   if (fileStatuss[i].getModificationTime()>result)
                       result = fileStatuss[i].getModificationTime();// make sure level n-1 is accounted for for delete partition case
               }
           long l = getHiveTableMaxModificationTs2(pathList.toArray(new Path[pathList.size()]),levelDeep-1);
           if (l>result) result = l;

       }
     return result;
   }

   public static String getFsDefaultName()
   {
      String uri = config_.get("fs.defaultFS");
      return uri;
   }


   public static boolean hdfsCreateDirectory(String pathStr) throws IOException
   {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsCreateDirectory()" + pathStr);
      Path dirPath = new Path(pathStr );
      FileSystem fs = FileSystem.get(dirPath.toUri(), config_);
      fs.mkdirs(dirPath);
      return true;
   }

   public static boolean hdfsRename(String fromPathStr, String toPathStr) throws IOException 
   {
      if (logger_.isDebugEnabled()) 
         logger_.debug("HDFSClient.hdfsRename(" + fromPathStr + ", " + toPathStr + ")");
      Path fromPath = new Path(fromPathStr );
      Path toPath = new Path(toPathStr );
      FileSystem fs = FileSystem.get(fromPath.toUri(), config_);
      fs.rename(fromPath, toPath);
      return true;
   }

   private native int sendFileStatus(long jniObj, int numFiles, int fileNo, boolean isDir, 
                        String filename, long modTime, long len,
                        short numReplicas, long blockSize, String owner, String group,
                        short permissions, long accessTime);

}



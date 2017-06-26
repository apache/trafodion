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

/**
 * 
 */
package org.trafodion.sql;

import java.io.IOException;
import java.io.OutputStream;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.sql.TrafConfiguration;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.FSUtils;
import org.apache.hadoop.hbase.util.HFileArchiveUtil;
import org.apache.hadoop.io.IOUtils;
import org.apache.hadoop.io.SequenceFile;
import org.apache.hadoop.io.Writable;
import org.apache.hadoop.io.ByteWritable;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.compress.CodecPool;
import org.apache.hadoop.io.compress.CompressionCodec;
import org.apache.hadoop.io.compress.Compressor;
import org.apache.hadoop.io.compress.GzipCodec;
import org.apache.hadoop.io.SequenceFile.CompressionType;
import org.apache.hadoop.util.ReflectionUtils;
import org.apache.hadoop.io.compress.*;
import org.apache.hadoop.io.compress.zlib.*;
import org.apache.hadoop.fs.*;

import java.io.*;
import java.util.List;

import org.apache.hadoop.util.*;
import org.apache.hadoop.io.*;
import org.apache.log4j.Logger;

import com.google.common.collect.Lists;
import com.google.protobuf.ServiceException;

import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.fs.permission.AclEntry;
import org.apache.hadoop.fs.permission.FsPermission;
public class SequenceFileWriter {

    static Logger logger = Logger.getLogger(SequenceFileWriter.class.getName());
    static Configuration conf = null;           // File system configuration
    
    SequenceFile.Writer writer = null;

    FSDataOutputStream fsOut = null;
    OutputStream outStream = null;
    boolean sameStream = true;

    FileSystem  fs = null;
    /**
     * Class Constructor
     */
    static {
       conf = TrafConfiguration.create(TrafConfiguration.HDFS_CONF);
    }
    SequenceFileWriter() throws IOException
    {
    }
	
    public String open(String path) throws IOException {
        Path filename = new Path(path);
        writer = SequenceFile.createWriter(conf, 
          	       SequenceFile.Writer.file(filename),
          	       SequenceFile.Writer.keyClass(ByteWritable.class),
          	       SequenceFile.Writer.valueClass(BytesWritable.class),
          	       SequenceFile.Writer.compression(CompressionType.NONE));
        return null;
    }
	
    public String open(String path, int compressionType) throws IOException	{
        Path filename = new Path(path);
        
        CompressionType compType=null;
        switch (compressionType) {
          case 0:
            compType = CompressionType.NONE;
            break;
            
          case 1:
            compType = CompressionType.RECORD;
            break;
            
          case 2:
            compType = CompressionType.BLOCK;
            break;
          
          default:
            throw new IOException("Wrong argument for compression type.");
        }   
        writer = SequenceFile.createWriter(conf, 
          	                               SequenceFile.Writer.file(filename),
          	                               SequenceFile.Writer.keyClass(BytesWritable.class),
          	                               SequenceFile.Writer.valueClass(Text.class),
          	                               SequenceFile.Writer.compression(compType));
        return null;
    }
	
    public String write(String data) throws IOException {
        if (writer == null)
           throw new IOException("open() was not called first.");
			
	writer.append(new BytesWritable(), new Text(data.getBytes()));
        return null;
    }
	
    public String close() throws IOException {
        if (writer != null) {
           writer.close();
           writer = null;
        }
        return null;
    }
    
    boolean hdfsCreate(String fname , boolean compress) throws IOException
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() - started" );
      Path filePath = null;
      if (!compress || (compress && fname.endsWith(".gz")))
        filePath = new Path(fname);
      else
        filePath = new Path(fname + ".gz");
        
      fs = FileSystem.get(filePath.toUri(),conf);
      fsOut = fs.create(filePath, true);
      
      outStream = fsOut;
      
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() - file created" );
      if (compress)
      {
        GzipCodec gzipCodec = (GzipCodec) ReflectionUtils.newInstance( GzipCodec.class, conf);
        Compressor gzipCompressor = CodecPool.getCompressor(gzipCodec);
        outStream = gzipCodec.createOutputStream(fsOut, gzipCompressor);
        sameStream = false;
      }
      
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() - compressed output stream created" );
      return true;
    }
    
    boolean hdfsWrite(byte[] buff, long len) throws IOException
    {

      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsWrite() - started" );
      outStream.write(buff);
      outStream.flush();
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsWrite() - bytes written and flushed:" + len  );
      return true;
    }
    
    boolean hdfsClose() throws IOException
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsClose() - started" );
      if (sameStream) { 
         if (outStream != null) {
            outStream.close();
            outStream = null;
         }
         fsOut = null;
      }
      else {
         if (outStream != null) {
            outStream.close();
            outStream = null;
         }
         if (fsOut != null) {
            fsOut.close();
            fsOut = null;
         }
      }
      return true;
    }

    
    public boolean hdfsMergeFiles(String srcPathStr, String dstPathStr) throws IOException
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - start");
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - source Path: " + srcPathStr + 
                                               ", destination File:" + dstPathStr );
        Path srcPath = new Path(srcPathStr );
        srcPath = srcPath.makeQualified(srcPath.toUri(), null);
        FileSystem srcFs = FileSystem.get(srcPath.toUri(),conf);
  
        Path dstPath = new Path(dstPathStr);
        dstPath = dstPath.makeQualified(dstPath.toUri(), null);
        FileSystem dstFs = FileSystem.get(dstPath.toUri(),conf);
        
        if (dstFs.exists(dstPath))
        {
          if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - destination files exists" );
          // for this prototype we just delete the file-- will change in next code drops
          dstFs.delete(dstPath, false);
           // The caller should already have checked existence of file-- throw exception 
           //throw new FileAlreadyExistsException(dstPath.toString());
        }
        
        Path tmpSrcPath = new Path(srcPath, "tmp");

        FileSystem.mkdirs(srcFs, tmpSrcPath,srcFs.getFileStatus(srcPath).getPermission());
        logger.debug("SequenceFileWriter.hdfsMergeFiles() - tmp folder created." );
        Path[] files = FileUtil.stat2Paths(srcFs.listStatus(srcPath));
        for (Path f : files)
        {
          srcFs.rename(f, tmpSrcPath);
        }
        // copyMerge and use false for the delete option since it removes the whole directory
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - copyMerge" );
        FileUtil.copyMerge(srcFs, tmpSrcPath, dstFs, dstPath, false, conf, null);
        
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - delete intermediate files" );
        srcFs.delete(tmpSrcPath, true);
      return true;
    }

    public boolean hdfsCleanUnloadPath(String uldPathStr
                         /*, boolean checkExistence, String mergeFileStr*/) throws IOException
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCleanUnloadPath() - start");
      logger.debug("SequenceFileWriter.hdfsCleanUnloadPath() - unload Path: " + uldPathStr );
      
      Path uldPath = new Path(uldPathStr );
      uldPath = uldPath.makeQualified(uldPath.toUri(), null);
      FileSystem srcFs = FileSystem.get(uldPath.toUri(),conf);
      if (!srcFs.exists(uldPath))
      {
        //unload location does not exist. hdfscreate will create it later
        //nothing to do 
        logger.debug("SequenceFileWriter.hdfsCleanUnloadPath() -- unload location does not exist." );
        return true;
      }
       
      Path[] files = FileUtil.stat2Paths(srcFs.listStatus(uldPath));
      logger.debug("SequenceFileWriter.hdfsCleanUnloadPath() - delete files" );
      for (Path f : files){
        srcFs.delete(f, false);
      }
      return true;
    }

  public boolean hdfsExists(String filePathStr) throws IOException 
  {
    logger.debug("SequenceFileWriter.hdfsExists() - start");
    logger.debug("SequenceFileWriter.hdfsExists() - Path: " + filePathStr);

        //check existence of the merge Path
       Path filePath = new Path(filePathStr );
       filePath = filePath.makeQualified(filePath.toUri(), null);
       FileSystem mergeFs = FileSystem.get(filePath.toUri(),conf);
       if (mergeFs.exists( filePath))
       {
       logger.debug("SequenceFileWriter.hdfsExists() - Path: "
       + filePath + " exists" );
         return true;
       }
    return false;
  }

  public boolean hdfsDeletePath(String pathStr) throws IOException
  {
    if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsDeletePath() - start - Path: " + pathStr);
      Path delPath = new Path(pathStr );
      delPath = delPath.makeQualified(delPath.toUri(), null);
      FileSystem fs = FileSystem.get(delPath.toUri(),conf);
      fs.delete(delPath, true);
    return true;
  }
}

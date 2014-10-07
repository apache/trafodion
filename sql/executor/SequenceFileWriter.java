// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

/**
 * 
 */
package org.trafodion.sql.HBaseAccess;

import java.io.IOException;
import java.io.OutputStream;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.util.Bytes;
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

import org.apache.hadoop.util.*;
import org.apache.hadoop.io.*;
import org.apache.log4j.Logger;


public class SequenceFileWriter {

    static Logger logger = Logger.getLogger(SequenceFileWriter.class.getName());
    Configuration conf = null;           // File system configuration
    SequenceFile.Writer writer = null;

    FSDataOutputStream fsOut = null;
    OutputStream compressedOut = null;
    
    FileSystem  fs = null;
    /**
     * Class Constructor
     */
    SequenceFileWriter() {
      conf = new Configuration();
      conf.set("fs.hdfs.impl","org.apache.hadoop.hdfs.DistributedFileSystem");
    }
    
	
    public String open(String path)	{
      try {
        Path filename = new Path(path);
        writer = SequenceFile.createWriter(conf, 
          	       SequenceFile.Writer.file(filename),
          	       SequenceFile.Writer.keyClass(ByteWritable.class),
          	       SequenceFile.Writer.valueClass(BytesWritable.class),
          	       SequenceFile.Writer.compression(CompressionType.NONE));
        return null;
      } catch (Exception e) {
        //e.printStackTrace();
        return e.getMessage();
      }	
    }
	
    public String open(String path, int compressionType)	{
      try {
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
            return "Wrong argument for compression type.";
        }
        
        writer = SequenceFile.createWriter(conf, 
          	                               SequenceFile.Writer.file(filename),
          	                               SequenceFile.Writer.keyClass(BytesWritable.class),
          	                               SequenceFile.Writer.valueClass(Text.class),
          	                               SequenceFile.Writer.compression(compType));
        return null;
      } catch (Exception e) {
        //e.printStackTrace();
        return e.getMessage();
      }	
    }
	
    public String write(String data) {
		  if (writer == null)
			  return "open() was not called first.";
			
      try {
	      writer.append(new BytesWritable(), new Text(data.getBytes()));
        return null;
    	} catch (IOException e) {
    	  //e.printStackTrace();
        return e.getMessage();
    	}
    }
	
    public String close() {
		  if (writer == null)
			  return "open() was not called first.";
			
      try {
        writer.close();
        return null;
      } catch (Exception e) {
        //e.printStackTrace();
        return e.getMessage();
      }
    }
    
    
    
    boolean hdfsCreate(String fname) throws IOException
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() - started" );
      Path filePath = new Path(fname + ".gz");
      fs = FileSystem.get(filePath.toUri(),conf);
      fsOut = fs.create(filePath, true);
      
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() - file created" );
      
      GzipCodec gzipCodec = (GzipCodec) ReflectionUtils.newInstance( GzipCodec.class, conf);
      Compressor gzipCompressor = CodecPool.getCompressor(gzipCodec);
      try 
      {
         compressedOut = gzipCodec.createOutputStream(fsOut, gzipCompressor);
      }
      catch (IOException e)
      {
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() --exception :" + e);
        throw e;
      }
      
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() - compressed output stream created" );
      return true;
    }
    
    boolean hdfsWrite(String buff, long len) throws Exception
    {

      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsWrite() - started" );
      try
      {
        compressedOut.write(Bytes.toBytes(buff), 0, (int)len);
        compressedOut.flush();
      }
      catch (Exception e)
      {
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsWrite() -- exception: " + e);
        throw e;
      }
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsWrite() - bytes written and flushed:" + len  );
      
      return true;
    }
    
    boolean hdfsClose() throws IOException
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsClose() - started" );
      try
      {
        compressedOut.close();
        fsOut.close();
      }
      catch (IOException e)
      {
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsClose() - exception:" + e);
        throw e;
      }
      return true;
    }

    
    public boolean hdfsMergeFiles(String srcPathStr, String dstPathStr) throws Exception
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - start");
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - source Path: " + srcPathStr + 
                                               ", destination File:" + dstPathStr );
      try 
      {
        Path srcPath = new Path(srcPathStr );
        srcPath = srcPath.makeQualified(srcPath.toUri(), null);
        FileSystem srcFs = FileSystem.get(srcPath.toUri(),conf);
  
        Path dstPath = new Path(dstPathStr);
        dstPath = dstPath.makeQualified(dstPath.toUri(), null);
        FileSystem dstFs = FileSystem.get(dstPath.toUri(),conf);
        
        if (dstFs.exists(dstPath)){
          if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - destination files exists" );
          // for this prototype we just delete the file-- will change in next code drops
          dstFs.delete(dstPath, false);
           // The caller should already have checked existence of file-- throw exception 
           //throw new FileAlreadyExistsException(dstPath.toString());
        }
        
        // copyMerge and use false for the delete option since it removes the whole directory
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - copyMerge" );
        FileUtil.copyMerge(srcFs, srcPath, dstFs, dstPath, false, conf, null);
        
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - delete intermediate files" );
        Path[] files = FileUtil.stat2Paths(srcFs.listStatus(srcPath));
        for (Path f : files){
          srcFs.delete(f, false);
        }
      }
      catch (IOException e)
      {
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() --exception:" + e);
        throw e;
      }
      
      
      return true;
    }
    public boolean hdfsCleanUnloadPath(String uldPathStr, boolean checkExistence, String mergeFileStr) throws Exception
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCleanUnloadPath() - start");
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - unload Path: " + uldPathStr );
      
      try 
      {
      if (checkExistence){
        Path mergePath = new Path(mergeFileStr );
        mergePath = mergePath.makeQualified(mergePath.toUri(), null);
        FileSystem mergeFs = FileSystem.get(mergePath.toUri(),conf);
        if (mergeFs.exists( mergePath)){
          if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - marge Path: " + mergePath + " already exists" );
          throw new FileAlreadyExistsException(mergePath.toString());
        }
      }
      
      Path uldPath = new Path(uldPathStr );
      uldPath = uldPath.makeQualified(uldPath.toUri(), null);
      FileSystem srcFs = FileSystem.get(uldPath.toUri(),conf);

      Path[] files = FileUtil.stat2Paths(srcFs.listStatus(uldPath));
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() - delete files" );
      for (Path f : files){
        srcFs.delete(f, false);
      }
      }
      catch (IOException e)
      {
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() -exception:" + e);
        throw e;
      }
      
      return true;
    }

}

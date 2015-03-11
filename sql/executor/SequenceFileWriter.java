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
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.TableSnapshotScanner;
import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos.SnapshotDescription;
import org.apache.hadoop.hbase.snapshot.SnapshotCreationException;
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
    Configuration conf = null;           // File system configuration
    HBaseAdmin admin = null;
    
    SequenceFile.Writer writer = null;

    FSDataOutputStream fsOut = null;
    OutputStream outStream = null;
    
    FileSystem  fs = null;
    /**
     * Class Constructor
     */
    SequenceFileWriter() throws MasterNotRunningException, ZooKeeperConnectionException, ServiceException, IOException
    {
      init("", "");
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
        try 
        {
          outStream = gzipCodec.createOutputStream(fsOut, gzipCompressor);
        }
        catch (IOException e)
        {
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() --exception :" + e);
          throw e;
        }
      }
      
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCreate() - compressed output stream created" );
      return true;
    }
    
    boolean hdfsWrite(byte[] buff, long len) throws Exception,OutOfMemoryError
    {

      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsWrite() - started" );
      try
      {
        outStream.write(buff);
        outStream.flush();
      }
      catch (Exception e)
      {
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsWrite() -- exception: " + e);
        throw e;
      }
      catch (OutOfMemoryError e1)
      {
        logger.debug("SequenceFileWriter.hdfsWrite() -- OutOfMemory Error: " + e1);
        throw e1;
      }
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsWrite() - bytes written and flushed:" + len  );
      
      return true;
    }
    
    boolean hdfsClose() throws IOException
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsClose() - started" );
      try
      {
        outStream.close();
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
      }
      catch (IOException e)
      {
        if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsMergeFiles() --exception:" + e);
        throw e;
      }
      
      
      return true;
    }
    public boolean hdfsCleanUnloadPath(String uldPathStr
                         /*, boolean checkExistence, String mergeFileStr*/) throws Exception
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsCleanUnloadPath() - start");
      logger.debug("SequenceFileWriter.hdfsCleanUnloadPath() - unload Path: " + uldPathStr );
      
      try 
      {
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
      }
      catch (IOException e)
      {
        logger.debug("SequenceFileWriter.hdfsCleanUnloadPath() -exception:" + e);
        throw e;
      }
      
      return true;
    }

  public boolean hdfsExists(String filePathStr) throws Exception 
  {
    logger.debug("SequenceFileWriter.hdfsExists() - start");
    logger.debug("SequenceFileWriter.hdfsExists() - Path: " + filePathStr);

    try 
    {
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

    } catch (IOException e) {
      logger.debug("SequenceFileWriter.hdfsExists() -exception:" + e);
      throw e;
    }
    return false;
  }

  public boolean hdfsDeletePath(String pathStr) throws Exception
  {
    if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsDeletePath() - start - Path: " + pathStr);
    try 
    {
      Path delPath = new Path(pathStr );
      delPath = delPath.makeQualified(delPath.toUri(), null);
      FileSystem fs = FileSystem.get(delPath.toUri(),conf);
      fs.delete(delPath, true);
    }
    catch (IOException e)
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.hdfsDeletePath() --exception:" + e);
      throw e;
    }
    
    return true;
  }

  private boolean init(String zkServers, String zkPort) 
      throws MasterNotRunningException, ZooKeeperConnectionException, ServiceException, IOException
  {
    logger.debug("SequenceFileWriter.init(" + zkServers + ", " + zkPort + ") called.");
    if (conf != null)
      return true;
    
    conf = HBaseConfiguration.create();
    if (zkServers.length() > 0)
      conf.set("hbase.zookeeper.quorum", zkServers);
    if (zkPort.length() > 0)
      conf.set("hbase.zookeeper.property.clientPort", zkPort);
    HBaseAdmin.checkHBaseAvailable(conf);
    return true;
  }
  
  public boolean createSnapshot( String tableName, String snapshotName)
      throws MasterNotRunningException, IOException, SnapshotCreationException, 
      InterruptedException, ZooKeeperConnectionException, ServiceException
  {
    try 
    {
      if (admin == null)
        admin = new HBaseAdmin(conf);
      admin.snapshot(snapshotName, tableName);
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.createSnapshot() - Snapshot created: " + snapshotName);
    }
    catch (Exception e)
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.createSnapshot() - Exception: " + e);
      throw e;
    }
    return true;
  }
  public boolean verifySnapshot( String tableName, String snapshotName)
      throws MasterNotRunningException, IOException, SnapshotCreationException, 
      InterruptedException, ZooKeeperConnectionException, ServiceException
  {
    try 
    {
      if (admin == null)
        admin = new HBaseAdmin(conf);
      List<SnapshotDescription>  lstSnaps = admin.listSnapshots();

      for (SnapshotDescription snpd : lstSnaps) 
      {
        if (snpd.getName().compareTo(snapshotName) == 0 && 
            snpd.getTable().compareTo(tableName) == 0)
        {
          if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.verifySnapshot() - Snapshot verified: " + snapshotName);
          return true;
        }
      }
    }
    catch (Exception e)
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.verifySnapshot() - Exception: " + e);
      throw e;
    }
    return false;
  }
 
  public boolean deleteSnapshot( String snapshotName)
      throws MasterNotRunningException, IOException, SnapshotCreationException, 
      InterruptedException, ZooKeeperConnectionException, ServiceException
  {
    try 
    {
      if (admin == null)
        admin = new HBaseAdmin(conf);
      admin.deleteSnapshot(snapshotName);
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.deleteSnapshot() - Snapshot deleted: " + snapshotName);
    }
    catch (Exception e)
    {
      if (logger.isDebugEnabled()) logger.debug("SequenceFileWriter.deleteSnapshot() - Exception: " + e);
      throw e;
    }

    return true;
  }

  public boolean release()  throws IOException
  {
    if (admin != null)
    {
      admin.close();
      admin = null;
    }
    return true;
  }
}
